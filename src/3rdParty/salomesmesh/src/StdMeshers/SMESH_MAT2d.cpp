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
// File      : SMESH_MAT2d.cxx
// Created   : Thu May 28 17:49:53 2015
// Author    : Edward AGAPOV (eap)

#include "SMESH_MAT2d.hxx"

#include <list>

#include <BRepAdaptor_CompCurve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_B2d.hxx>
//#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_TangentialDeflection.hxx>
// #include <GCPnts_UniformAbscissa.hxx>
// #include <GCPnts_UniformDeflection.hxx>
#include <Geom2d_Curve.hxx>
//#include <GeomAdaptor_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <TopExp.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

#ifdef _DEBUG_
#define _MYDEBUG_
#include "SMESH_File.hxx"
#include "SMESH_Comment.hxx"
#endif

using namespace std;
using boost::polygon::x;
using boost::polygon::y;
using SMESH_MAT2d::TVD;
using SMESH_MAT2d::TVDEdge;
using SMESH_MAT2d::TVDCell;
using SMESH_MAT2d::TVDVertex;

namespace
{
  // Input data for construct_voronoi()
  // -------------------------------------------------------------------------------------

  struct InPoint
  {
    int _a, _b;    // coordinates
    double _param; // param on EDGE
    InPoint(int x, int y, double param) : _a(x), _b(y), _param(param) {}
    InPoint() : _a(0), _b(0), _param(0) {}

    // working data
    list< const TVDEdge* > _edges; // MA edges of a concave InPoint in CCW order

    size_t index( const vector< InPoint >& inPoints ) const { return this - &inPoints[0]; }
    bool operator==( const InPoint& other ) const { return _a == other._a && _b == other._b; }
    bool operator==( const TVDVertex* v ) const { return ( Abs( _a - v->x() ) < 1. &&
                                                           Abs( _b - v->y() ) < 1. ); }
  };
  // -------------------------------------------------------------------------------------

  struct InSegment
  {
    InPoint * _p0;
    InPoint * _p1;

    // working data
    size_t                 _geomEdgeInd; // EDGE index within the FACE
    const TVDCell*         _cell;
    list< const TVDEdge* > _edges; // MA edges in CCW order within _cell

    InSegment( InPoint * p0, InPoint * p1, size_t iE)
      : _p0(p0), _p1(p1), _geomEdgeInd(iE) {}
    InSegment() : _p0(0), _p1(0), _geomEdgeInd(0) {}

    const InPoint& point0() const { return *_p0; }
    const InPoint& point1() const { return *_p1; }

    inline bool isConnected( const TVDEdge* edge );

    inline bool isExternal( const TVDEdge* edge );

    static void setGeomEdgeToCell( const TVDCell* cell, size_t eID ) { cell->color( eID ); }

    static size_t getGeomEdge( const TVDCell* cell ) { return cell->color(); }
  };

  // check  if a TVDEdge begins at my end or ends at my start
  inline bool InSegment::isConnected( const TVDEdge* edge )
  {
    return (( edge->vertex0() && edge->vertex1() )
            &&
            ((Abs( edge->vertex0()->x() - _p1->_a ) < 1.&&
              Abs( edge->vertex0()->y() - _p1->_b ) < 1.  ) ||
             (Abs( edge->vertex1()->x() - _p0->_a ) < 1.&&
              Abs( edge->vertex1()->y() - _p0->_b ) < 1.  )));
  }

  // check if a MA TVDEdge is outside of a domain
  inline bool InSegment::isExternal( const TVDEdge* edge )
  {
    double dot = // x1*x2 + y1*y2;   (x1,y1) - internal normal of InSegment
      ( _p0->_b - _p1->_b ) * ( 0.5 * ( edge->vertex0()->x() + edge->vertex1()->x() ) - _p0->_a ) +
      ( _p1->_a - _p0->_a ) * ( 0.5 * ( edge->vertex0()->y() + edge->vertex1()->y() ) - _p0->_b );
    return dot < 0.;
  }

  // // -------------------------------------------------------------------------------------
  // const size_t theExternMA = 111; // to mark external MA edges

  // bool isExternal( const TVDEdge* edge )
  // {
  //   return ( SMESH_MAT2d::Branch::getBndSegment( edge ) == theExternMA );
  // }

  // // mark external MA edges
  // void markExternalEdges( const TVDEdge* edge )
  // {
  //   if ( isExternal( edge ))
  //     return;
  //   SMESH_MAT2d::Branch::setBndSegment( theExternMA, edge );
  //   SMESH_MAT2d::Branch::setBndSegment( theExternMA, edge->twin() );
  //   if ( edge->is_primary() && edge->vertex1() )
  //   {
  //     const TVDVertex * v = edge->vertex1();
  //     edge = v->incident_edge();
  //     do {
  //       markExternalEdges( edge );
  //       edge = edge->rot_next();
  //     } while ( edge != v->incident_edge() );
  //   }
  // }

  // -------------------------------------------------------------------------------------
#ifdef _MYDEBUG_
  // writes segments into a txt file readable by voronoi_visualizer
  void inSegmentsToFile( vector< InSegment>& inSegments)
  {
    if ( inSegments.size() > 1000 )
      return;
    const char* fileName = "/misc/dn25/salome/eap/salome/misc/Code/C++/MAdebug.txt";
    SMESH_File file(fileName, false );
    file.remove();
    file.openForWriting();
    SMESH_Comment text;
    text << "0\n"; // nb points
    text << inSegments.size() << "\n"; // nb segments
    for ( size_t i = 0; i < inSegments.size(); ++i )
    {
      text << inSegments[i]._p0->_a << " "
           << inSegments[i]._p0->_b << " "
           << inSegments[i]._p1->_a << " "
           << inSegments[i]._p1->_b << "\n";
    }
    text << "\n";
    file.write( text.c_str(), text.size() );
    cout << "Write " << fileName << endl;
  }
  void dumpEdge( const TVDEdge* edge )
  {
    cout << "*Edge_" << edge;
    if ( !edge->vertex0() )
      cout << " ( INF, INF";
    else
      cout << " ( " << edge->vertex0()->x() << ", " << edge->vertex0()->y();
    if ( !edge->vertex1() )
      cout << ") -> ( INF, INF";
    else
      cout << ") -> ( " << edge->vertex1()->x() << ", " << edge->vertex1()->y();
    cout << ")\t cell=" << edge->cell()
         << " iBnd=" << edge->color()
         << " twin=" << edge->twin()
         << " twin_cell=" << edge->twin()->cell()
         << " prev=" << edge->prev() << " next=" << edge->next()
         << ( edge->is_primary() ? " MA " : " SCND" )
         << ( edge->is_linear() ? " LIN " : " CURV" )
         << endl;
  }
  void dumpCell( const TVDCell* cell )
  {
    cout << "**Cell_" << cell << " GEOM=" << cell->color() << " ";
    cout << ( cell->contains_segment() ? " SEG " : " PNT " );
    if ( cell-> is_degenerate() )
      cout << " degen ";
    else
    {
      cout << endl;
      const TVDEdge* edge = cell->incident_edge();
      size_t i = 0;
      do {
        edge = edge->next();
        cout << "   - " << ++i << " ";
        dumpEdge( edge );
      } while (edge != cell->incident_edge());
    }
  }
#else
  inline void inSegmentsToFile( vector< InSegment>& inSegments) {}
  inline void dumpEdge( const TVDEdge* edge ) {}
  inline void dumpCell( const TVDCell* cell ) {}
#endif
}
// -------------------------------------------------------------------------------------

namespace boost {
  namespace polygon {

    template <>
    struct geometry_concept<InPoint> {
      typedef point_concept type;
    };
    template <>
    struct point_traits<InPoint> {
      typedef int coordinate_type;

      static inline coordinate_type get(const InPoint& point, orientation_2d orient) {
        return (orient == HORIZONTAL) ? point._a : point._b;
      }
    };

    template <>
    struct geometry_concept<InSegment> {
      typedef segment_concept type;
    };

    template <>
    struct segment_traits<InSegment> {
      typedef int coordinate_type;
      typedef InPoint point_type;

      static inline point_type get(const InSegment& segment, direction_1d dir) {
        return *(dir.to_int() ? segment._p1 : segment._p0);
      }
    };
  }  // namespace polygon
} // namespace boost
  // -------------------------------------------------------------------------------------

namespace
{
  const int    theNoBrachID = 0;
  double       theScale[2]; // scale used in bndSegsToMesh()
  const size_t theNoEdgeID = std::numeric_limits<size_t>::max() / 1000;

  // -------------------------------------------------------------------------------------
  /*!
   * \brief Intermediate DS to create InPoint's
   */
  struct UVU
  {
    gp_Pnt2d _uv;
    double   _u;
    UVU( gp_Pnt2d uv, double u ): _uv(uv), _u(u) {}
    InPoint getInPoint( double scale[2] )
    {
      return InPoint( int( _uv.X() * scale[0]), int( _uv.Y() * scale[1]), _u );
    }
  };
  // -------------------------------------------------------------------------------------
  /*!
   * \brief Segment of EDGE, used to create BndPoints
   */
  struct BndSeg
  {
    InSegment*       _inSeg;
    const TVDEdge*   _edge;
    double           _uLast;
    BndSeg*          _prev; // previous BndSeg in FACE boundary
    int              _branchID; // negative ID means reverse direction

    BndSeg( InSegment* seg, const TVDEdge* edge, double u ):
      _inSeg(seg), _edge(edge), _uLast(u), _prev(0), _branchID( theNoBrachID ) {}

    void setIndexToEdge( size_t id )
    {
      SMESH_MAT2d::Branch::setBndSegment( id, _edge );
    }

    int branchID() const { return Abs( _branchID ); }

    size_t geomEdge() const { return _inSeg->_geomEdgeInd; }

    static BndSeg* getBndSegOfEdge( const TVDEdge*              edge,
                                    vector< vector< BndSeg > >& bndSegsPerEdge )
    {
      BndSeg* seg = 0;
      if ( edge )
      {
        size_t oppSegIndex  = SMESH_MAT2d::Branch::getBndSegment( edge );
        size_t oppEdgeIndex = SMESH_MAT2d::Branch::getGeomEdge  ( edge );
        if ( oppEdgeIndex < bndSegsPerEdge.size() &&
             oppSegIndex < bndSegsPerEdge[ oppEdgeIndex ].size() )
        {
          seg = & bndSegsPerEdge[ oppEdgeIndex ][ oppSegIndex ];
        }
      }
      return seg;
    }

    void setBranch( int branchID, vector< vector< BndSeg > >& bndSegsPerEdge )
    {
      _branchID = branchID;

      // pass branch to an opposite BndSeg
      if ( _edge )
        if ( BndSeg* oppSeg = getBndSegOfEdge( _edge->twin(), bndSegsPerEdge ))
        {
          if ( oppSeg->_branchID == theNoBrachID )
            oppSeg->_branchID = -branchID;
        }
    }
    bool hasOppositeEdge()
    {
      if ( !_edge ) return false;
      return ( _inSeg->getGeomEdge( _edge->twin()->cell() ) != theNoEdgeID );
    }

    // check a next segment in CW order
    bool isSameBranch( const BndSeg& seg2 )
    {
      if ( !_edge || !seg2._edge )
        return true;

      if ( _edge->twin() == seg2._edge )
        return true;

      const TVDCell* cell1 = this->_edge->twin()->cell();
      const TVDCell* cell2 = seg2. _edge->twin()->cell();
      if ( cell1 == cell2 )
        return true;

      const TVDEdge* edgeMedium1 = this->_edge->twin()->next();
      const TVDEdge* edgeMedium2 = seg2. _edge->twin()->prev();

      if ( edgeMedium1->is_secondary() && edgeMedium2->is_secondary() )
      {
        if ( edgeMedium1->twin() == edgeMedium2 )
          return true;
        // edgeMedium's are edges whose twin()->cell is built on an end point of inSegment
        // and is located between cell1 and cell2
        if ( edgeMedium1->twin() == edgeMedium2->twin() ) // is this possible???
          return true;
        if ( edgeMedium1->twin() == edgeMedium2->twin()->next() &&
             edgeMedium1->twin()->cell()->contains_point() )
          return true;
      }
      else if ( edgeMedium1->is_primary() && edgeMedium2->is_primary() )
      {
        if ( edgeMedium1->twin() == edgeMedium2 &&
             SMESH_MAT2d::Branch::getGeomEdge( edgeMedium1 ) ==
             SMESH_MAT2d::Branch::getGeomEdge( edgeMedium2 ))
          // this is an ignored MA edge between inSegment's on one EDGE forming a convex corner
          return true;
      }

      return false;
    }
  }; // struct BndSeg

  // -------------------------------------------------------------------------------------
  /*!
   * \brief Iterator 
   */
  struct BranchIterator
  {
    int                                 _i, _size;
    const std::vector<const TVDEdge*> & _edges;
    bool                                _closed;

    BranchIterator(const std::vector<const TVDEdge*> & edges, int i )
      :_i( i ), _size( edges.size() ), _edges( edges )
    {
      _closed = ( edges[0]->vertex1() == edges.back()->vertex0() || // closed branch
                  edges[0]->vertex0() == edges.back()->vertex1() );
    }
    const TVDEdge* operator++() { ++_i; return edge(); }
    const TVDEdge* operator--() { --_i; return edge(); }
    bool operator<( const BranchIterator& other ) { return _i < other._i; }
    BranchIterator& operator=( const BranchIterator& other ) { _i = other._i; return *this; }
    void set(int i) { _i = i; }
    int  index() const { return _i; }
    int  indexMod() const { return ( _i + _size ) % _size; }
    const TVDEdge* edge() const {
      return _closed ? _edges[ indexMod() ] : ( _i < 0 || _i >= _size ) ? 0 : _edges[ _i ];
    }
    const TVDEdge* edgePrev() { --_i; const TVDEdge* e = edge(); ++_i; return e; }
    const TVDEdge* edgeNext() { ++_i; const TVDEdge* e = edge(); --_i; return e; }
  };

  //================================================================================
  /*!
   * \brief debug: to visually check found MA edges
   */
  //================================================================================

  void bndSegsToMesh( const vector< vector< BndSeg > >& bndSegsPerEdge )
  {
#ifdef _MYDEBUG_
    if ( !getenv("bndSegsToMesh")) return;
    map< const TVDVertex *, int > v2Node;
    map< const TVDVertex *, int >::iterator v2n;
    set< const TVDEdge* > addedEdges;

    const char* fileName = "/misc/dn25/salome/eap/salome/misc/Code/C++/MAedges.py";
    SMESH_File file(fileName, false );
    file.remove();
    file.openForWriting();
    SMESH_Comment text;
    text << "import salome, SMESH\n";
    text << "salome.salome_init()\n";
    text << "from salome.smesh import smeshBuilder\n";
    text << "smesh = smeshBuilder.New(salome.myStudy)\n";
    text << "m=smesh.Mesh()\n";
    for ( size_t iE = 0; iE < bndSegsPerEdge.size(); ++iE )
    {
      const vector< BndSeg >& bndSegs = bndSegsPerEdge[ iE ];
      for ( size_t i = 0; i < bndSegs.size(); ++i )
      {
        if ( !bndSegs[i]._edge )
          text << "# E=" << iE << " i=" << i << " NULL edge\n";
        else if ( !bndSegs[i]._edge->vertex0() ||
                  !bndSegs[i]._edge->vertex1() )
          text << "# E=" << iE << " i=" << i << " INFINITE edge\n";
        else if ( addedEdges.insert( bndSegs[i]._edge ).second &&
                  addedEdges.insert( bndSegs[i]._edge->twin() ).second )
        {
          v2n = v2Node.insert( make_pair( bndSegs[i]._edge->vertex0(), v2Node.size() + 1 )).first;
          int n0 = v2n->second;
          if ( n0 == v2Node.size() )
            text << "n" << n0 << " = m.AddNode( "
                 << bndSegs[i]._edge->vertex0()->x() / theScale[0] << ", "
                 << bndSegs[i]._edge->vertex0()->y() / theScale[1] << ", 0 )\n";

          v2n = v2Node.insert( make_pair( bndSegs[i]._edge->vertex1(), v2Node.size() + 1 )).first;
          int n1 = v2n->second;
          if ( n1 == v2Node.size() )
            text << "n" << n1 << " = m.AddNode( "
                 << bndSegs[i]._edge->vertex1()->x() / theScale[0] << ", "
                 << bndSegs[i]._edge->vertex1()->y() / theScale[1] << ", 0 )\n";

          text << "e" << i << " = m.AddEdge([ n" << n0 << ", n" << n1 << " ])\n";
        }
      }
    }
    text << "\n";
    file.write( text.c_str(), text.size() );
    cout << "execfile( '" << fileName << "')" << endl;
#endif
  }

  //================================================================================
  /*!
   * \brief Computes length of a TVDEdge
   */
  //================================================================================

  double length( const TVDEdge* edge )
  {
    gp_XY d( edge->vertex0()->x() - edge->vertex1()->x(),
             edge->vertex0()->y() - edge->vertex1()->y() );
    return d.Modulus();
  }

  //================================================================================
  /*!
   * \brief Compute scale to have the same 2d proportions as in 3d
   */
  //================================================================================

  void computeProportionScale( const TopoDS_Face& face,
                               const Bnd_B2d&     uvBox,
                               double             scale[2])
  {
    scale[0] = scale[1] = 1.;
    if ( uvBox.IsVoid() ) return;

    TopLoc_Location loc;
    Handle(Geom_Surface) surface = BRep_Tool::Surface( face, loc );

    const int nbDiv = 30;
    gp_XY uvMin = uvBox.CornerMin(), uvMax = uvBox.CornerMax();
    gp_XY uvMid = 0.5 * ( uvMin + uvMax );
    double du = ( uvMax.X() - uvMin.X() ) / nbDiv;
    double dv = ( uvMax.Y() - uvMin.Y() ) / nbDiv;

    double uLen3d = 0, vLen3d = 0;
    gp_Pnt uPrevP = surface->Value( uvMin.X(), uvMid.Y() );
    gp_Pnt vPrevP = surface->Value( uvMid.X(), uvMin.Y() );
    for (int i = 1; i <= nbDiv; i++)
    {
      double u = uvMin.X() + du * i;
      double v = uvMin.Y() + dv * i;
      gp_Pnt uP = surface->Value( u, uvMid.Y() );
      gp_Pnt vP = surface->Value( uvMid.X(), v );
      uLen3d += uP.Distance( uPrevP );
      vLen3d += vP.Distance( vPrevP );
      uPrevP = uP;
      vPrevP = vP;
    }
    scale[0] = uLen3d / ( uvMax.X() - uvMin.X() );
    scale[1] = vLen3d / ( uvMax.Y() - uvMin.Y() );
  }

  //================================================================================
  /*!
   * \brief Fill input data for construct_voronoi()
   */
  //================================================================================

  bool makeInputData(const TopoDS_Face&                face,
                     const std::vector< TopoDS_Edge >& edges,
                     const double                      minSegLen,
                     vector< InPoint >&                inPoints,
                     vector< InSegment>&               inSegments,
                     double                            scale[2])
  {
    const double theDiscrCoef = 0.5; // to decrease minSegLen for discretization
    TopLoc_Location loc;

    // discretize the EDGEs to get 2d points and segments

    vector< vector< UVU > > uvuVec( edges.size() );
    Bnd_B2d uvBox;
    for ( size_t iE = 0; iE < edges.size(); ++iE )
    {
      vector< UVU > & points = uvuVec[ iE ];

      double f,l;
      Handle(Geom_Curve)   c3d = BRep_Tool::Curve         ( edges[ iE ], loc, f, l );
      Handle(Geom2d_Curve) c2d = BRep_Tool::CurveOnSurface( edges[ iE ], face, f, l );
      if ( c2d.IsNull() ) return false;

      points.push_back( UVU( c2d->Value( f ), f ));
      uvBox.Add( points.back()._uv );

      Geom2dAdaptor_Curve c2dAdaptor (c2d, f,l );
      double curDeflect = 0.3; //0.01;  //Curvature deflection
      double angDeflect = 0.2; // 0.09; //Angular deflection

      GCPnts_TangentialDeflection discret(c2dAdaptor, angDeflect, curDeflect);
      // if ( discret.NbPoints() > 2 )
      // {
      //   cout << endl;
      //   do
      //   {
      //     discret.Initialize( c2dAdaptor, 100, curDeflect );
      //     cout << "C " << curDeflect << " " << discret.NbPoints() << endl;
      //     curDeflect *= 1.5;
      //   }
      //   while ( discret.NbPoints() > 5 );
      //   cout << endl;
      //   do
      //   {
      //     discret.Initialize( c2dAdaptor, angDeflect, 100 );
      //     cout << "A " << angDeflect << " " << discret.NbPoints() << endl;
      //     angDeflect *= 1.5;
      //   }
      //   while ( discret.NbPoints() > 5 );
      // }
      gp_Pnt p, pPrev;
      if ( !c3d.IsNull() )
        pPrev = c3d->Value( f );
      if ( discret.NbPoints() > 2 )
        for ( int i = 2; i <= discret.NbPoints(); i++ ) // skip the 1st point
        {
          double u = discret.Parameter(i);
          if ( !c3d.IsNull() )
          {
            p = c3d->Value( u );
            int nbDiv = int( p.Distance( pPrev ) / minSegLen / theDiscrCoef );
            double dU = ( u - points.back()._u ) / nbDiv;
            for ( int iD = 1; iD < nbDiv; ++iD )
            {
              double uD = points.back()._u + dU;
              points.push_back( UVU( c2d->Value( uD ), uD ));
            }
            pPrev = p;
          }
          points.push_back( UVU( c2d->Value( u ), u ));
          uvBox.Add( points.back()._uv );
        }
      // if ( !c3d.IsNull() )
      // {
      //   vector<double> params;
      //   GeomAdaptor_Curve c3dAdaptor( c3d,f,l );
      //   if ( useDefl )
      //   {
      //     const double deflection = minSegLen * 0.1;
      //     GCPnts_UniformDeflection discret( c3dAdaptor, deflection, f, l, true );
      //     if ( !discret.IsDone() )
      //       return false;
      //     int nbP = discret.NbPoints();
      //     for ( int i = 2; i < nbP; i++ ) // skip 1st and last points
      //       params.push_back( discret.Parameter(i) );
      //   }
      //   else
      //   {
      //     double   eLen = GCPnts_AbscissaPoint::Length( c3dAdaptor );
      //     int     nbSeg = Max( 1, int( eLen / minSegLen / theDiscrCoef ));
      //     double segLen = eLen / nbSeg;
      //     GCPnts_UniformAbscissa discret( c3dAdaptor, segLen, f, l );
      //     int nbP = Min( discret.NbPoints(), nbSeg + 1 );
      //     for ( int i = 2; i < nbP; i++ ) // skip 1st and last points
      //       params.push_back( discret.Parameter(i) );
      //   }
      //   for ( size_t i = 0; i < params.size(); ++i )
      //   {
      //     points.push_back( UVU( c2d->Value( params[i] ), params[i] ));
      //     uvBox.Add( points.back()._uv );
      //   }
      // }
      if ( points.size() < 2 )
      {
        points.push_back( UVU( c2d->Value( l ), l ));
        uvBox.Add( points.back()._uv );
      }
      if ( edges[ iE ].Orientation() == TopAbs_REVERSED )
        std::reverse( points.begin(), points.end() );
    }

    // make connected EDGEs have same UV at shared VERTEX
    TopoDS_Vertex vShared;
    for ( size_t iE = 0; iE < edges.size(); ++iE )
    {
      size_t iE2 = (iE+1) % edges.size();
      if ( !TopExp::CommonVertex( edges[iE], edges[iE2], vShared ))
        continue;
      if ( !vShared.IsSame( TopExp::LastVertex( edges[iE], true )))
        return false;
      vector< UVU > & points1 = uvuVec[ iE ];
      vector< UVU > & points2 = uvuVec[ iE2 ];
      gp_Pnt2d & uv1 = points1.back() ._uv;
      gp_Pnt2d & uv2 = points2.front()._uv;
      uv1 = uv2 = 0.5 * ( uv1.XY() + uv2.XY() );
    }

    // get scale to have the same 2d proportions as in 3d
    computeProportionScale( face, uvBox, scale );

    // make scale to have coordinates precise enough when converted to int

    gp_XY uvMin = uvBox.CornerMin(), uvMax = uvBox.CornerMax();

    uvMin.SetX(uvMin.X() * scale[0]);
    uvMin.SetY(uvMin.Y() * scale[1]);
    uvMax.SetX(uvMax.X() * scale[0]);
    uvMax.SetY(uvMax.Y() * scale[1]);

    double vMax[2] = { Max( Abs( uvMin.X() ), Abs( uvMax.X() )),
                       Max( Abs( uvMin.Y() ), Abs( uvMax.Y() )) };
    int iMax = ( vMax[0] > vMax[1] ) ? 0 : 1;
    const double precision = 1e-5;
    double preciScale = Min( vMax[iMax] / precision,
                             std::numeric_limits<int>::max() / vMax[iMax] );
    preciScale /= scale[iMax];
    double roundedScale = 10; // to ease debug
    while ( roundedScale * 10 < preciScale )
      roundedScale *= 10.;
    scale[0] *= roundedScale;
    scale[1] *= roundedScale;

    // create input points and segments

    inPoints.clear();
    inSegments.clear();
    size_t nbPnt = 0;
    for ( size_t iE = 0; iE < uvuVec.size(); ++iE )
      nbPnt += uvuVec[ iE ].size();
    inPoints.resize( nbPnt );
    inSegments.reserve( nbPnt );

    size_t iP = 0;
    if ( face.Orientation() == TopAbs_REVERSED )
    {
      for ( int iE = uvuVec.size()-1; iE >= 0; --iE )
      {
        vector< UVU > & points = uvuVec[ iE ];
        inPoints[ iP++ ] = points.back().getInPoint( scale );
        for ( size_t i = points.size()-1; i >= 1; --i )
        {
          inPoints[ iP++ ] = points[i-1].getInPoint( scale );
          inSegments.push_back( InSegment( & inPoints[ iP-2 ], & inPoints[ iP-1 ], iE ));
        }
      }
    }
    else
    {
      for ( size_t iE = 0; iE < uvuVec.size(); ++iE )
      {
        vector< UVU > & points = uvuVec[ iE ];
        inPoints[ iP++ ] = points[0].getInPoint( scale );
        for ( size_t i = 1; i < points.size(); ++i )
        {
          inPoints[ iP++ ] = points[i].getInPoint( scale );
          inSegments.push_back( InSegment( & inPoints[ iP-2 ], & inPoints[ iP-1 ], iE ));
        }
      }
    }
    // debug
    theScale[0] = scale[0];
    theScale[1] = scale[1];

    return true;
  }

  //================================================================================
  /*!
   * \brief Update a branch joined to another one
   */
  //================================================================================

  void updateJoinedBranch( vector< const TVDEdge* > &   branchEdges,
                           const size_t                 newID,
                           vector< vector< BndSeg > > & bndSegs,
                           const bool                   reverse)
  {
    BndSeg *seg1, *seg2;
    if ( reverse )
    {
      for ( size_t i = 0; i < branchEdges.size(); ++i )
      {
        if (( seg1 = BndSeg::getBndSegOfEdge( branchEdges[i],         bndSegs )) &&
            ( seg2 = BndSeg::getBndSegOfEdge( branchEdges[i]->twin(), bndSegs )))
        {
          seg1->_branchID /= seg1->branchID();
          seg2->_branchID /= seg2->branchID();
          seg1->_branchID *= -newID;
          seg2->_branchID *= -newID;
          branchEdges[i] = branchEdges[i]->twin();
        }
      }
      std::reverse( branchEdges.begin(), branchEdges.end() );
    }
    else
    {
      for ( size_t i = 0; i < branchEdges.size(); ++i )
      {
        if (( seg1 = BndSeg::getBndSegOfEdge( branchEdges[i],         bndSegs )) &&
            ( seg2 = BndSeg::getBndSegOfEdge( branchEdges[i]->twin(), bndSegs )))
        {
          seg1->_branchID /= seg1->branchID();
          seg2->_branchID /= seg2->branchID();
          seg1->_branchID *= newID;
          seg2->_branchID *= newID;
        }
      }
    }
  }

  //================================================================================
  /*!
   * \brief Create MA branches and FACE boundary data
   *  \param [in] vd - voronoi diagram of \a inSegments
   *  \param [in] inPoints - FACE boundary points
   *  \param [in,out] inSegments - FACE boundary segments
   *  \param [out] branch - MA branches to fill
   *  \param [out] branchEnd - ends of MA branches to fill
   *  \param [out] boundary - FACE boundary to fill
   */
  //================================================================================

  void makeMA( const TVD&                               vd,
               const bool                               ignoreCorners,
               vector< InPoint >&                       inPoints,
               vector< InSegment > &                    inSegments,
               vector< SMESH_MAT2d::Branch >&           branch,
               vector< const SMESH_MAT2d::BranchEnd* >& branchPnt,
               SMESH_MAT2d::Boundary&                   boundary )
  {
    // Associate MA cells with geom EDGEs
    for (TVD::const_cell_iterator it = vd.cells().begin(); it != vd.cells().end(); ++it)
    {
      const TVDCell* cell = &(*it);
      if ( cell->contains_segment() )
      {
        InSegment& seg = inSegments[ cell->source_index() ];
        seg._cell = cell;
        seg.setGeomEdgeToCell( cell, seg._geomEdgeInd );
      }
      else
      {
        InSegment::setGeomEdgeToCell( cell, theNoEdgeID );
      }
    }

    vector< bool > inPntChecked( inPoints.size(), false );

    // Find MA edges of each inSegment

    for ( size_t i = 0; i < inSegments.size(); ++i )
    {
      InSegment& inSeg = inSegments[i];

      // get edges around the cell lying on MA
      bool hasSecondary = false;
      const TVDEdge* edge = inSeg._cell->incident_edge();
      do {
        edge = edge->next(); // Returns the CCW next edge within the cell.
        if ( edge->is_primary() && !inSeg.isExternal( edge ) )
          inSeg._edges.push_back( edge ); // edge equidistant from two InSegments
        else
          hasSecondary = true;
      } while (edge != inSeg._cell->incident_edge());

      // there can be several continuous MA edges but maEdges can begin in the middle of
      // a chain of continuous MA edges. Make the chain continuous.
      list< const TVDEdge* >& maEdges = inSeg._edges;
      if ( maEdges.empty() )
        continue;
      if ( hasSecondary )
        while ( maEdges.back()->next() == maEdges.front() )
          maEdges.splice( maEdges.end(), maEdges, maEdges.begin() );

      // remove maEdges equidistant from two neighbor InSegments of the same geom EDGE
      list< const TVDEdge* >::iterator e = maEdges.begin();
      while ( e != maEdges.end() )
      {
        const TVDCell* cell2 = (*e)->twin()->cell(); // cell on the other side of a MA edge
        size_t         geoE2 = InSegment::getGeomEdge( cell2 );
        bool        toRemove = ( inSeg._geomEdgeInd == geoE2 && inSeg.isConnected( *e ));
        if ( toRemove )
          e = maEdges.erase( e );
        else
          ++e;
      }
      if ( maEdges.empty() )
        continue;

      // add MA edges corresponding to concave InPoints
      for ( int is2nd = 0; is2nd < 2; ++is2nd ) // loop on two ends of inSeg
      {
        InPoint& inPnt = *( is2nd ? inSeg._p1 : inSeg._p0 );
        size_t    pInd = inPnt.index( inPoints );
        if ( inPntChecked[ pInd ] )
          continue;
        if ( pInd > 0 &&
             inPntChecked[ pInd-1 ] &&
             inPoints[ pInd-1 ] == inPnt )
          continue;
        inPntChecked[ pInd ] = true;

        const TVDEdge* maE = is2nd ? maEdges.front() : maEdges.back();
        if ( inPnt == ( is2nd ? maE->vertex0() : maE->vertex1() ))
          continue;
        const TVDEdge* edge =  // a secondary TVDEdge connecting inPnt and maE
          is2nd ? maE->prev() : maE->next();
        while ( inSeg.isConnected( edge ))
        {
          if ( edge->is_primary() ) break; // this should not happen
          const TVDEdge* edge2 = edge->twin(); // we are in a neighbor cell, add MA edges to inPnt
          if ( inSeg.getGeomEdge( edge2->cell() ) != theNoEdgeID )
            break; // cell of an InSegment
          bool hasInfinite = false;
          list< const TVDEdge* > pointEdges;
          edge = edge2;
          do
          {
            edge = edge->next(); // Returns the CCW next edge within the cell.
            if ( edge->is_infinite() )
              hasInfinite = true;
            else if ( edge->is_primary() && !inSeg.isExternal( edge ))
              pointEdges.push_back( edge );
          }
          while ( edge != edge2 && !hasInfinite );

          if ( hasInfinite || pointEdges.empty() )
            break;
          inPnt._edges.splice( inPnt._edges.end(), pointEdges );
          inSeg.setGeomEdgeToCell( edge->cell(), inSeg._geomEdgeInd );

          edge = is2nd ? inPnt._edges.front()->prev() : inPnt._edges.back()->next();
        }
      } // add MA edges corresponding to concave InPoints

    } // loop on inSegments to find corresponding MA edges


    // -------------------------------------------
    // Create Branches and BndPoints for each EDGE
    // -------------------------------------------

    if ( inPoints.front() == inPoints.back() /*&& !inPoints[0]._edges.empty()*/ )
    {
      inPntChecked[0] = false; // do not use the 1st point twice
      //InSegment::setGeomEdgeToCell( inPoints[0]._edges.back()->cell(), theNoEdgeID );
      inPoints[0]._edges.clear();
    }

    // Divide InSegment's into BndSeg's (so that each BndSeg corresponds to one MA edge)

    vector< vector< BndSeg > > bndSegsPerEdge( boundary.nbEdges() ); // all BndSeg's
    {
      vector< BndSeg > bndSegs; // bndSeg's of a current EDGE
      size_t prevGeomEdge = theNoEdgeID;

      list< const TVDEdge* >::reverse_iterator e;
      for ( size_t i = 0; i < inSegments.size(); ++i )
      {
        InSegment& inSeg = inSegments[i];

        if ( inSeg._geomEdgeInd != prevGeomEdge )
        {
          if ( !bndSegs.empty() )
            bndSegsPerEdge[ prevGeomEdge ].swap( bndSegs );
          prevGeomEdge = inSeg._geomEdgeInd;
        }

        // segments around 1st concave point
        size_t ip0 = inSeg._p0->index( inPoints );
        if ( inPntChecked[ ip0 ] )
          for ( e = inSeg._p0->_edges.rbegin(); e != inSeg._p0->_edges.rend(); ++e )
            bndSegs.push_back( BndSeg( &inSeg, *e, inSeg._p0->_param ));
        inPntChecked[ ip0 ] = false;

        // segments of InSegment's
        const size_t nbMaEdges = inSeg._edges.size();
        switch ( nbMaEdges ) {
        case 0: // "around" circle center
          bndSegs.push_back( BndSeg( &inSeg, 0, inSeg._p1->_param )); break;
        case 1:
          bndSegs.push_back( BndSeg( &inSeg, inSeg._edges.back(), inSeg._p1->_param )); break;
        default:
          gp_XY inSegDir( inSeg._p1->_a - inSeg._p0->_a,
                          inSeg._p1->_b - inSeg._p0->_b );
          const double inSegLen2 = inSegDir.SquareModulus();
          e = inSeg._edges.rbegin();
          for ( size_t iE = 1; iE < nbMaEdges; ++e, ++iE )
          {
            gp_XY toMA( (*e)->vertex0()->x() - inSeg._p0->_a,
                        (*e)->vertex0()->y() - inSeg._p0->_b );
            double r = toMA * inSegDir / inSegLen2;
            double u = r * inSeg._p1->_param + ( 1. - r ) * inSeg._p0->_param;
            bndSegs.push_back( BndSeg( &inSeg, *e, u ));
          }
          bndSegs.push_back( BndSeg( &inSeg, *e, inSeg._p1->_param ));
        }
        // segments around 2nd concave point
        size_t ip1 = inSeg._p1->index( inPoints );
        if ( inPntChecked[ ip1 ] )
          for ( e = inSeg._p1->_edges.rbegin(); e != inSeg._p1->_edges.rend(); ++e )
            bndSegs.push_back( BndSeg( &inSeg, *e, inSeg._p1->_param ));
        inPntChecked[ ip1 ] = false;
      }
      if ( !bndSegs.empty() )
        bndSegsPerEdge[ prevGeomEdge ].swap( bndSegs );
    }

    // prepare to MA branch search
    for ( size_t iE = 0; iE < bndSegsPerEdge.size(); ++iE )
    {
      // 1) make TVDEdge's know it's BndSeg to enable passing branchID to
      // an opposite BndSeg in BndSeg::setBranch(); geom EDGE ID is known from TVDCell
      // 2) connect bndSegs via BndSeg::_prev

      vector< BndSeg >& bndSegs = bndSegsPerEdge[ iE ];
      if ( bndSegs.empty() ) continue;

      for ( size_t i = 1; i < bndSegs.size(); ++i )
      {
        bndSegs[i]._prev = & bndSegs[i-1];
        bndSegs[i].setIndexToEdge( i );
      }
      // look for the last bndSeg of previous EDGE to set bndSegs[0]._prev
      const InPoint& p0 = bndSegs[0]._inSeg->point0();
      for ( size_t iE2 = 0; iE2 < bndSegsPerEdge.size(); ++iE2 )
        if ( p0 == bndSegsPerEdge[ iE2 ].back()._inSeg->point1() )
        {
          bndSegs[0]._prev = & bndSegsPerEdge[ iE2 ].back();
          break;
        }
      bndSegs[0].setIndexToEdge( 0 );
    }

    bndSegsToMesh( bndSegsPerEdge ); // debug: visually check found MA edges


    // Find TVDEdge's of Branches and associate them with bndSegs

    vector< vector<const TVDEdge*> > branchEdges;
    branchEdges.reserve( boundary.nbEdges() * 4 );

    map< const TVDVertex*, SMESH_MAT2d::BranchEndType > endType;

    int branchID = 1; // we code orientation as branchID sign
    branchEdges.resize( branchID );

    for ( size_t iE = 0; iE < bndSegsPerEdge.size(); ++iE )
    {
      vector< BndSeg >& bndSegs = bndSegsPerEdge[ iE ];
      for ( size_t i = 0; i < bndSegs.size(); ++i )
      {
        if ( bndSegs[i].branchID() )
        {
          if ( bndSegs[i]._prev &&
               bndSegs[i]._branchID == -bndSegs[i]._prev->_branchID &&
               bndSegs[i]._edge )
          {
            SMESH_MAT2d::BranchEndType type =
              ( bndSegs[i]._inSeg->isConnected( bndSegs[i]._edge ) ?
                SMESH_MAT2d::BE_ON_VERTEX :
                SMESH_MAT2d::BE_END );
            endType.insert( make_pair( bndSegs[i]._edge->vertex1(), type ));
          }
          continue;
        }
        if ( !bndSegs[i]._prev &&
             !bndSegs[i].hasOppositeEdge() )
          continue;

        if ( !bndSegs[i]._prev ||
             !bndSegs[i]._prev->isSameBranch( bndSegs[i] ))
        {
          branchEdges.resize(( branchID = branchEdges.size()) + 1 );
          if ( bndSegs[i]._edge && bndSegs[i]._prev )
            endType.insert( make_pair( bndSegs[i]._edge->vertex1(), SMESH_MAT2d::BE_BRANCH_POINT ));
        }
        else if ( bndSegs[i]._prev->_branchID )
        {
          branchID = bndSegs[i]._prev->_branchID;  // with sign
        }
        else if ( bndSegs[i]._edge ) // 1st bndSeg of a WIRE
        {
          branchEdges.resize(( branchID = branchEdges.size()) + 1 );
          if ( bndSegs[i]._inSeg->isConnected( bndSegs[i]._edge ))
          {
            if ( bndSegs[i]._inSeg->point0() == bndSegs[i]._edge->vertex1() )
              endType.insert( make_pair( bndSegs[i]._edge->vertex1(), SMESH_MAT2d::BE_ON_VERTEX ));
            else
              endType.insert( make_pair( bndSegs[i]._edge->vertex0(), SMESH_MAT2d::BE_ON_VERTEX ));
          }
        }

        bndSegs[i].setBranch( branchID, bndSegsPerEdge ); // set to i-th and to the opposite bndSeg
        if ( bndSegs[i].hasOppositeEdge() )
          branchEdges[ bndSegs[i].branchID() ].push_back( bndSegs[i]._edge );
      }
    }

    // join the 1st and the last branch edges if it is the same branch
    // if ( bndSegs.back().branchID() != bndSegs.front().branchID() &&
    //      bndSegs.back().isSameBranch( bndSegs.front() ))
    // {
    //   vector<const TVDEdge*> & br1 = branchEdges[ bndSegs.front().branchID() ];
    //   vector<const TVDEdge*> & br2 = branchEdges[ bndSegs.back().branchID()  ];
    //   br1.insert( br1.begin(), br2.begin(), br2.end() );
    //   br2.clear();
    // }

    // remove branches ending at BE_ON_VERTEX

    vector<bool> isBranchRemoved( branchEdges.size(), false );

    if ( ignoreCorners && branchEdges.size() > 2 && !branchEdges[2].empty() )
    {
      // find branches to remove
      map< const TVDVertex*, SMESH_MAT2d::BranchEndType >::iterator v2et;
      for ( size_t iB = 1; iB < branchEdges.size(); ++iB )
      {
        if ( branchEdges[iB].empty() )
          continue;
        const TVDVertex* v0 = branchEdges[iB][0]->vertex1();
        const TVDVertex* v1 = branchEdges[iB].back()->vertex0();
        v2et = endType.find( v0 );
        if ( v2et != endType.end() && v2et->second == SMESH_MAT2d::BE_ON_VERTEX )
          isBranchRemoved[ iB ] = true;
        v2et = endType.find( v1 );
        if ( v2et != endType.end() && v2et->second == SMESH_MAT2d::BE_ON_VERTEX )
          isBranchRemoved[ iB ] = true;
      }
      // try to join not removed branches into one
      for ( size_t iB = 1; iB < branchEdges.size(); ++iB )
      {
        if ( branchEdges[iB].empty() || isBranchRemoved[iB] )
          continue;
        const TVDVertex* v0 = branchEdges[iB][0]->vertex1();
        const TVDVertex* v1 = branchEdges[iB].back()->vertex0();
        v2et = endType.find( v0 );
        if ( v2et == endType.end() || v2et->second != SMESH_MAT2d::BE_BRANCH_POINT )
          v0 = 0;
        v2et = endType.find( v1 );
        if ( v2et == endType.end() || v2et->second != SMESH_MAT2d::BE_BRANCH_POINT )
          v1 = 0;
        if ( !v0 && !v1 )
          continue;

        for ( int isV0 = 0; isV0 < 2; ++isV0 )
        {
          const TVDVertex* v = isV0 ? v0 : v1;
          size_t iBrToJoin = 0;
          for ( size_t iB2 = 1; iB2 < branchEdges.size(); ++iB2 )
          {
            if ( branchEdges[iB2].empty() || isBranchRemoved[iB2] || iB == iB2 )
              continue;
            const TVDVertex* v02 = branchEdges[iB2][0]->vertex1();
            const TVDVertex* v12 = branchEdges[iB2].back()->vertex0();
            if ( v == v02 || v == v12 )
            {
              if ( iBrToJoin > 0 )
              {
                iBrToJoin = 0;
                break; // more than 2 not removed branches meat at a TVDVertex
              }
              iBrToJoin = iB2;
            }
          }
          if ( iBrToJoin > 0 )
          {
            vector<const TVDEdge*>& branch = branchEdges[ iBrToJoin ];
            const TVDVertex* v02 = branch[0]->vertex1();
            const TVDVertex* v12 = branch.back()->vertex0();
            updateJoinedBranch( branch, iB, bndSegsPerEdge, /*reverse=*/(v0 == v02 || v1 == v12 ));
            if ( v0 == v02 || v0 == v12 )
              branchEdges[iB].insert( branchEdges[iB].begin(), branch.begin(), branch.end() );
            else
              branchEdges[iB].insert( branchEdges[iB].end(),   branch.begin(), branch.end() );
            branch.clear();
          }
        }
      } // loop on branchEdges
    } // if ( ignoreCorners )

    // associate branchIDs and the input branch vector (arg)
    vector< SMESH_MAT2d::Branch* > branchByID( branchEdges.size(), 0 );
    int nbBranches = 0;
    for ( size_t i = 0; i < branchEdges.size(); ++i )
    {
      nbBranches += ( !branchEdges[i].empty() );
    }
    branch.resize( nbBranches );
    size_t iBr = 0;
    for ( size_t brID = 1; brID < branchEdges.size(); ++brID ) // 1st - not removed
    {
      if ( !branchEdges[ brID ].empty() && !isBranchRemoved[ brID ])
        branchByID[ brID ] = & branch[ iBr++ ];
    }
    for ( size_t brID = 1; brID < branchEdges.size(); ++brID ) // then - removed
    {
      if ( !branchEdges[ brID ].empty() && isBranchRemoved[ brID ])
        branchByID[ brID ] = & branch[ iBr++ ];
    }

    // Fill in BndPoints of each EDGE of the boundary

    //size_t iSeg = 0;
    int edgeInd = -1, dInd = 0;
    for ( size_t iE = 0; iE < bndSegsPerEdge.size(); ++iE )
    {
      vector< BndSeg >&          bndSegs = bndSegsPerEdge[ iE ];
      SMESH_MAT2d::BndPoints & bndPoints = boundary.getPoints( iE );

      // make TVDEdge know an index of bndSegs within BndPoints
      for ( size_t i = 0; i < bndSegs.size(); ++i )
        if ( bndSegs[i]._edge )
          SMESH_MAT2d::Branch::setBndSegment( i, bndSegs[i]._edge );

      // parameters on EDGE

      bndPoints._params.reserve( bndSegs.size() + 1 );
      bndPoints._params.push_back( bndSegs[ 0 ]._inSeg->_p0->_param );

      for ( size_t i = 0; i < bndSegs.size(); ++i )
        bndPoints._params.push_back( bndSegs[ i ]._uLast );

      // MA edges

      bndPoints._maEdges.reserve( bndSegs.size() );

      for ( size_t i = 0; i < bndSegs.size(); ++i )
      {
        const size_t              brID = bndSegs[ i ].branchID();
        const SMESH_MAT2d::Branch*  br = branchByID[ brID ];

        if ( bndSegs[ i ]._edge && !branchEdges[ brID ].empty() )
        {
          edgeInd += dInd;

          if (( edgeInd < 0 ||
                edgeInd >= (int) branchEdges[ brID ].size() ) ||
              ( branchEdges[ brID ][ edgeInd ]         != bndSegs[ i ]._edge &&
                branchEdges[ brID ][ edgeInd ]->twin() != bndSegs[ i ]._edge ))
          {
            if ( bndSegs[ i ]._branchID < 0 )
            {
              dInd = -1;
              for ( edgeInd = branchEdges[ brID ].size() - 1; edgeInd > 0; --edgeInd )
                if ( branchEdges[ brID ][ edgeInd ]->twin() == bndSegs[ i ]._edge )
                  break;
            }
            else // bndSegs[ i ]._branchID > 0
            {
              dInd = +1;
              for ( edgeInd = 0; edgeInd < branchEdges[ brID ].size(); ++edgeInd )
                if ( branchEdges[ brID ][ edgeInd ] == bndSegs[ i ]._edge )
                  break;
            }
          }
        }
        else
        {
          // no MA edge, bndSeg corresponds to an end point of a branch
          if ( bndPoints._maEdges.empty() )
            edgeInd = 0;
          else
            edgeInd = branchEdges[ brID ].size();
          dInd = bndSegs[ i ]._branchID > 0 ? +1 : -1;
        }

        bndPoints._maEdges.push_back( make_pair( br, ( 1 + edgeInd ) * dInd ));

      } // loop on bndSegs of an EDGE
    } // loop on all bndSegs to construct Boundary

    // Initialize branches

    // find a not removed branch
    size_t iBrNorRemoved = 0;
    for ( size_t brID = 1; brID < branchEdges.size(); ++brID )
      if ( !branchEdges[brID].empty() && !isBranchRemoved[brID] )
      {
        iBrNorRemoved = brID;
        break;
      }
    // fill the branches with MA edges
    for ( size_t brID = 1; brID < branchEdges.size(); ++brID )
      if ( !branchEdges[brID].empty() )
      {
        branchByID[ brID ]->init( branchEdges[brID], & boundary, endType );
      }
    // mark removed branches
    for ( size_t brID = 1; brID < branchEdges.size(); ++brID )
      if ( isBranchRemoved[brID] && iBrNorRemoved > 0 )
      {
        SMESH_MAT2d::Branch* branch     = branchByID[ brID ];
        SMESH_MAT2d::Branch* mainBranch = branchByID[ iBrNorRemoved ];
        bool is1stBrPnt = ( branch->getEnd(0)->_type == SMESH_MAT2d::BE_BRANCH_POINT );
        const TVDVertex* branchVextex = 
          is1stBrPnt ? branch->getEnd(0)->_vertex : branch->getEnd(1)->_vertex;
        SMESH_MAT2d::BranchPoint bp = mainBranch->getPoint( branchVextex );
        branch->setRemoved( bp );
      }
    // set branches to branch ends
    for ( size_t i = 0; i < branch.size(); ++i )
      if ( !branch[i].isRemoved() )
        branch[i].setBranchesToEnds( branch );

    // fill branchPnt arg
    map< const TVDVertex*, const SMESH_MAT2d::BranchEnd* > v2end;
    for ( size_t i = 0; i < branch.size(); ++i )
    {
      if ( branch[i].getEnd(0)->_branches.size() > 2 )
        v2end.insert( make_pair( branch[i].getEnd(0)->_vertex, branch[i].getEnd(0) ));
      if ( branch[i].getEnd(1)->_branches.size() > 2 )
        v2end.insert( make_pair( branch[i].getEnd(1)->_vertex, branch[i].getEnd(1) ));
    }
    branchPnt.resize( v2end.size() );
    map< const TVDVertex*, const SMESH_MAT2d::BranchEnd* >::iterator v2e = v2end.begin();
    for ( size_t i = 0; v2e != v2end.end(); ++v2e, ++i )
      branchPnt[ i ] = v2e->second;

  } // makeMA()

} // namespace

//================================================================================
/*!
 * \brief MedialAxis constructor
 *  \param [in] face - a face to create MA for
 *  \param [in] edges - edges of the face (possibly not all) on the order they
 *              encounter in the face boundary.
 *  \param [in] minSegLen - minimal length of a mesh segment used to discretize
 *              the edges. It is used to define precision of MA approximation
 */
//================================================================================

SMESH_MAT2d::MedialAxis::MedialAxis(const TopoDS_Face&                face,
                                    const std::vector< TopoDS_Edge >& edges,
                                    const double                      minSegLen,
                                    const bool                        ignoreCorners):
  _face( face ), _boundary( edges.size() )
{
  // input to construct_voronoi()
  vector< InPoint >  inPoints;
  vector< InSegment> inSegments;
  if ( !makeInputData( face, edges, minSegLen, inPoints, inSegments, _scale ))
    return;

  inSegmentsToFile( inSegments );

  // build voronoi diagram
  construct_voronoi( inSegments.begin(), inSegments.end(), &_vd );

  // make MA data
  makeMA( _vd, ignoreCorners, inPoints, inSegments, _branch, _branchPnt, _boundary );

  // count valid branches
  _nbBranches = _branch.size();
  for ( size_t i = 0; i < _branch.size(); ++i )
    if ( _branch[i].isRemoved() )
      --_nbBranches;
}

//================================================================================
/*!
 * \brief Returns the i-th branch
 */
//================================================================================

const SMESH_MAT2d::Branch* SMESH_MAT2d::MedialAxis::getBranch(size_t i) const
{
  return i < _nbBranches ? &_branch[i] : 0;
}

//================================================================================
/*!
 * \brief Return UVs of ends of MA edges of a branch
 */
//================================================================================

void SMESH_MAT2d::MedialAxis::getPoints( const Branch*         branch,
                                         std::vector< gp_XY >& points) const
{
  branch->getPoints( points, _scale );
}

//================================================================================
/*!
 * \brief Returns a BranchPoint corresponding to a given point on a geom EDGE
 *  \param [in] iEdge - index of geom EDGE within a vector passed at MA construction
 *  \param [in] u - parameter of the point on EDGE curve
 *  \param [out] p - the found BranchPoint
 *  \return bool - is OK
 */
//================================================================================

bool SMESH_MAT2d::Boundary::getBranchPoint( const std::size_t iEdge,
                                            double            u,
                                            BranchPoint&      p ) const
{
  if ( iEdge >= _pointsPerEdge.size() || _pointsPerEdge[iEdge]._params.empty() )
    return false;

  const BndPoints& points = _pointsPerEdge[ iEdge ];
  const bool  edgeReverse = ( points._params[0] > points._params.back() );

  if ( u < ( edgeReverse ? points._params.back() : points._params[0] ))
    u = edgeReverse ? points._params.back() : points._params[0];
  else if ( u > ( edgeReverse ? points._params[0] : points._params.back()) )
    u = edgeReverse ? points._params[0] : points._params.back();

  double r = ( u - points._params[0] ) / ( points._params.back() - points._params[0] );
  int    i = int( r * double( points._maEdges.size()-1 ));
  if ( edgeReverse )
  {
    while ( points._params[i  ] < u ) --i;
    while ( points._params[i+1] > u ) ++i;
  }
  else
  {
    while ( points._params[i  ] > u ) --i;
    while ( points._params[i+1] < u ) ++i;
  }

  if ( points._params[i] == points._params[i+1] ) // coincident points at some end
  {
    int di = ( points._params[0] == points._params[i] ) ? +1 : -1;
    while ( points._params[i] == points._params[i+1] )
      i += di;
    if ( i < 0 || i+1 >= points._params.size() )
      i = 0;
  }

  double edgeParam = ( u - points._params[i] ) / ( points._params[i+1] - points._params[i] );

  if ( !points._maEdges[ i ].second ) // no branch at the EDGE end, look for a closest branch
  {
    if ( i < points._maEdges.size() / 2 ) // near 1st point
    {
      while ( i < points._maEdges.size()-1 && !points._maEdges[ i ].second )
        ++i;
      edgeParam = edgeReverse;
    }
    else // near last point
    {
      while ( i > 0 && !points._maEdges[ i ].second )
        --i;
      edgeParam = !edgeReverse;
    }
  }
  const std::pair< const Branch*, int >& maE = points._maEdges[ i ];
  bool maReverse = ( maE.second < 0 );

  p._branch    = maE.first;
  p._iEdge     = ( maReverse ? -maE.second : maE.second ) - 1; // countered from 1 to store sign
  p._edgeParam = ( maE.first && maReverse ) ? ( 1. - edgeParam ) : edgeParam;

  return true;
}

//================================================================================
/*!
 * \brief Returns a BranchPoint corresponding to a given BoundaryPoint on a geom EDGE
 *  \param [in] bp - the BoundaryPoint
 *  \param [out] p - the found BranchPoint
 *  \return bool - is OK
 */
//================================================================================

bool SMESH_MAT2d::Boundary::getBranchPoint( const BoundaryPoint& bp,
                                            BranchPoint&         p ) const
{
  return getBranchPoint( bp._edgeIndex, bp._param, p );
}

//================================================================================
/*!
 * \brief Check if a given boundary segment is a null-length segment on a concave
 *        boundary corner.
 *  \param [in] iEdge - index of a geom EDGE
 *  \param [in] iSeg - index of a boundary segment
 *  \return bool - true if the segment is on concave corner
 */
//================================================================================

bool SMESH_MAT2d::Boundary::isConcaveSegment( std::size_t iEdge, std::size_t iSeg ) const
{
  if ( iEdge >= _pointsPerEdge.size() || _pointsPerEdge[iEdge]._params.empty() )
    return false;

  const BndPoints& points = _pointsPerEdge[ iEdge ];
  if ( points._params.size() <= iSeg+1 )
    return false;

  return Abs( points._params[ iSeg ] - points._params[ iSeg+1 ]) < 1e-20;
}

//================================================================================
/*!
 * \brief Moves (changes _param) a given BoundaryPoint to a closest EDGE end
 */
//================================================================================

bool SMESH_MAT2d::Boundary::moveToClosestEdgeEnd( BoundaryPoint& bp ) const
{
  if ( bp._edgeIndex >= _pointsPerEdge.size() )
    return false;

  const BndPoints& points = _pointsPerEdge[ bp._edgeIndex ];
  if ( Abs( bp._param - points._params[0]) < Abs( points._params.back() - bp._param ))
    bp._param = points._params[0];
  else 
    bp._param = points._params.back();

  return true;
}

//================================================================================
/*!
 * \brief Creates a 3d curve corresponding to a Branch
 *  \param [in] branch - the Branch
 *  \return Adaptor3d_Curve* - the new curve the caller is to delete
 */
//================================================================================

Adaptor3d_Curve* SMESH_MAT2d::MedialAxis::make3DCurve(const Branch& branch) const
{
  Handle(Geom_Surface) surface = BRep_Tool::Surface( _face );
  if ( surface.IsNull() )
    return 0;

  vector< gp_XY > uv;
  branch.getPoints( uv, _scale );
  if ( uv.size() < 2 )
    return 0;

  vector< TopoDS_Vertex > vertex( uv.size() );
  for ( size_t i = 0; i < uv.size(); ++i )
    vertex[i] = BRepBuilderAPI_MakeVertex( surface->Value( uv[i].X(), uv[i].Y() ));

  TopoDS_Wire aWire;
  BRep_Builder aBuilder;
  aBuilder.MakeWire(aWire);
  for ( size_t i = 1; i < vertex.size(); ++i )
  {
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge( vertex[i-1], vertex[i] );
    aBuilder.Add( aWire, edge );
  }

  // if ( myEdge.size() == 2 && FirstVertex().IsSame( LastVertex() ))
  //   aWire.Closed(true); // issue 0021141

  return new BRepAdaptor_CompCurve( aWire );
}

//================================================================================
/*!
 * \brief Copy points of an EDGE
 */
//================================================================================

void SMESH_MAT2d::Branch::init( vector<const TVDEdge*>&                maEdges,
                                const Boundary*                        boundary,
                                map< const TVDVertex*, BranchEndType > endType )
{
  if ( maEdges.empty() ) return;

  _boundary = boundary;
  _maEdges.swap( maEdges );


  _params.reserve( _maEdges.size() + 1 );
  _params.push_back( 0. );
  for ( size_t i = 0; i < _maEdges.size(); ++i )
    _params.push_back( _params.back() + length( _maEdges[i] ));
  
  for ( size_t i = 1; i < _params.size(); ++i )
    _params[i] /= _params.back();


  _endPoint1._vertex = _maEdges.front()->vertex1();
  _endPoint2._vertex = _maEdges.back ()->vertex0();

  if ( endType.count( _endPoint1._vertex ))
    _endPoint1._type = endType[ _endPoint1._vertex ];
  if ( endType.count( _endPoint2._vertex ))
    _endPoint2._type = endType[ _endPoint2._vertex ];
}

//================================================================================
/*!
 * \brief fills BranchEnd::_branches of its ends
 */
//================================================================================

void SMESH_MAT2d::Branch::setBranchesToEnds( const vector< Branch >& branches )
{
  for ( size_t i = 0; i < branches.size(); ++i )
  {
    if ( this->_endPoint1._vertex == branches[i]._endPoint1._vertex ||
         this->_endPoint1._vertex == branches[i]._endPoint2._vertex )
      this->_endPoint1._branches.push_back( &branches[i] );

    if ( this->_endPoint2._vertex == branches[i]._endPoint1._vertex ||
         this->_endPoint2._vertex == branches[i]._endPoint2._vertex )
      this->_endPoint2._branches.push_back( &branches[i] );
  }
}

//================================================================================
/*!
 * \brief returns a BranchPoint corresponding to a TVDVertex
 */
//================================================================================

SMESH_MAT2d::BranchPoint SMESH_MAT2d::Branch::getPoint( const TVDVertex* vertex ) const
{
  BranchPoint p;
  p._branch = this;
  p._iEdge  = 0;

  if ( vertex == _maEdges[0]->vertex1() )
  {
    p._edgeParam = 0;
  }
  else
  {
    for ( ; p._iEdge < _maEdges.size(); ++p._iEdge )
      if ( vertex == _maEdges[ p._iEdge ]->vertex0() )
      {
        p._edgeParam = _params[ p._iEdge ];
        break;
      }
  }
  return p;
}

//================================================================================
/*!
 * \brief Sets a proxy point for a removed branch
 *  \param [in] proxyPoint - a point of another branch to which all points of this
 *         branch are mapped
 */
//================================================================================

void SMESH_MAT2d::Branch::setRemoved( const BranchPoint& proxyPoint )
{
  _proxyPoint = proxyPoint;
}

//================================================================================
/*!
 * \brief Returns points on two EDGEs, equidistant from a given point of this Branch
 *  \param [in] param - [0;1] normalized param on the Branch
 *  \param [out] bp1 - BoundaryPoint on EDGE with a lower index
 *  \param [out] bp2 - BoundaryPoint on EDGE with a higher index
 *  \return bool - true if the BoundaryPoint's found
 */
//================================================================================

bool SMESH_MAT2d::Branch::getBoundaryPoints(double         param,
                                            BoundaryPoint& bp1,
                                            BoundaryPoint& bp2 ) const
{
  if ( param < _params[0] || param > _params.back() )
    return false;

  // look for an index of a MA edge by param
  double ip = param * _params.size();
  size_t  i = size_t( Min( int( _maEdges.size()-1), int( ip )));

  while ( param < _params[i  ] ) --i;
  while ( param > _params[i+1] ) ++i;

  double r = ( param - _params[i] ) / ( _params[i+1] - _params[i] );

  return getBoundaryPoints( i, r, bp1, bp2 );
}

//================================================================================
/*!
 * \brief Returns points on two EDGEs, equidistant from a given point of this Branch
 *  \param [in] iMAEdge - index of a MA edge within this Branch
 *  \param [in] maEdgeParam - [0;1] normalized param on the \a iMAEdge
 *  \param [out] bp1 - BoundaryPoint on EDGE with a lower index
 *  \param [out] bp2 - BoundaryPoint on EDGE with a higher index
 *  \return bool - true if the BoundaryPoint's found
 */
//================================================================================

bool SMESH_MAT2d::Branch::getBoundaryPoints(std::size_t    iMAEdge,
                                            double         maEdgeParam,
                                            BoundaryPoint& bp1,
                                            BoundaryPoint& bp2 ) const
{
  if ( isRemoved() )
    return _proxyPoint._branch->getBoundaryPoints( _proxyPoint, bp1, bp2 );

  if ( iMAEdge > _maEdges.size() )
    return false;
  if ( iMAEdge == _maEdges.size() )
    iMAEdge = _maEdges.size() - 1;

  size_t iGeom1 = getGeomEdge( _maEdges[ iMAEdge ] );
  size_t iGeom2 = getGeomEdge( _maEdges[ iMAEdge ]->twin() );
  size_t iSeg1  = getBndSegment( _maEdges[ iMAEdge ] );
  size_t iSeg2  = getBndSegment( _maEdges[ iMAEdge ]->twin() );

  return ( _boundary->getPoint( iGeom1, iSeg1, maEdgeParam, bp1 ) &&
           _boundary->getPoint( iGeom2, iSeg2, maEdgeParam, bp2 ));
}

//================================================================================
/*!
 * \brief Returns points on two EDGEs, equidistant from a given point of this Branch
 */
//================================================================================

bool SMESH_MAT2d::Branch::getBoundaryPoints(const BranchPoint& p,
                                            BoundaryPoint&     bp1,
                                            BoundaryPoint&     bp2 ) const
{
  return ( p._branch ? p._branch : this )->getBoundaryPoints( p._iEdge, p._edgeParam, bp1, bp2 );
}

//================================================================================
/*!
 * \brief Return a parameter of a BranchPoint normalized within this Branch
 */
//================================================================================

bool SMESH_MAT2d::Branch::getParameter(const BranchPoint & p, double & u ) const
{
  if ( this != p._branch && p._branch )
    return p._branch->getParameter( p, u );

  if ( isRemoved() )
    return _proxyPoint._branch->getParameter( _proxyPoint, u );

  if ( p._iEdge > _params.size()-1 )
    return false;
  if ( p._iEdge == _params.size()-1 )
    return (u = 1.);

  u = ( _params[ p._iEdge   ] * ( 1 - p._edgeParam ) +
        _params[ p._iEdge+1 ] * p._edgeParam );

  return true;
}

//================================================================================
/*!
 * \brief Check type of both ends
 */
//================================================================================

bool SMESH_MAT2d::Branch::hasEndOfType(BranchEndType type) const
{
  return ( _endPoint1._type == type || _endPoint2._type == type );
}

//================================================================================
/*!
 * \brief Returns MA points
 *  \param [out] points - the 2d points
 *  \param [in] scale - the scale that was used to scale the 2d space of MA
 */
//================================================================================

void SMESH_MAT2d::Branch::getPoints( std::vector< gp_XY >& points,
                                     const double          scale[2]) const
{
  points.resize( _maEdges.size() + 1 );

  points[0].SetCoord( _maEdges[0]->vertex1()->x() / scale[0], // CCW order! -> vertex1 not vertex0
                      _maEdges[0]->vertex1()->y() / scale[1] );

  for ( size_t i = 0; i < _maEdges.size(); ++i )
    points[i+1].SetCoord( _maEdges[i]->vertex0()->x() / scale[0],
                          _maEdges[i]->vertex0()->y() / scale[1] );
}

//================================================================================
/*!
 * \brief Return indices of EDGEs equidistant from this branch
 */
//================================================================================

void SMESH_MAT2d::Branch::getGeomEdges( std::vector< std::size_t >& edgeIDs1,
                                        std::vector< std::size_t >& edgeIDs2 ) const
{
  edgeIDs1.push_back( getGeomEdge( _maEdges[0] ));
  edgeIDs2.push_back( getGeomEdge( _maEdges[0]->twin() ));

  for ( size_t i = 1; i < _maEdges.size(); ++i )
  {
    size_t ie1 = getGeomEdge( _maEdges[i] );
    size_t ie2 = getGeomEdge( _maEdges[i]->twin() );

    if ( edgeIDs1.back() != ie1 ) edgeIDs1.push_back( ie1 );
    if ( edgeIDs2.back() != ie2 ) edgeIDs2.push_back( ie2 );
  }
}

//================================================================================
/*!
 * \brief Looks for a BranchPoint position around a concave VERTEX
 */
//================================================================================

bool SMESH_MAT2d::Branch::addDivPntForConcaVertex( std::vector< std::size_t >&   edgeIDs1,
                                                   std::vector< std::size_t >&   edgeIDs2,
                                                   std::vector< BranchPoint >&   divPoints,
                                                   const vector<const TVDEdge*>& maEdges,
                                                   const vector<const TVDEdge*>& maEdgesTwin,
                                                   int &                         i) const
{
  // if there is a concave vertex between EDGEs
  // then position of a dividing BranchPoint is undefined, it is somewhere
  // on an arc-shaped part of the Branch around the concave vertex.
  // Chose this position by a VERTEX of the opposite EDGE, or put it in the middle
  // of the arc if there is no opposite VERTEX.
  // All null-length segments around a VERTEX belong to one of EDGEs.

  BranchPoint divisionPnt;
  divisionPnt._branch = this;

  BranchIterator iCur( maEdges, i );

  size_t ie1 = getGeomEdge( maEdges    [i] );
  size_t ie2 = getGeomEdge( maEdgesTwin[i] );

  size_t iSeg1  = getBndSegment( iCur.edgePrev() );
  size_t iSeg2  = getBndSegment( iCur.edge() );
  bool isConcaPrev = _boundary->isConcaveSegment( edgeIDs1.back(), iSeg1 );
  bool isConcaNext = _boundary->isConcaveSegment( ie1,             iSeg2 );
  if ( !isConcaNext && !isConcaPrev )
    return false;

  bool isConcaveV = false;

  const TVDEdge* maE;
  BranchIterator iPrev( maEdges, i ), iNext( maEdges, i );
   --iPrev;
  if ( isConcaNext ) // all null-length segments follow
  {
    // look for a VERTEX of the opposite EDGE
    // iNext - next after all null-length segments
    while ( ( maE = ++iNext ))
    {
      iSeg2 = getBndSegment( maE );
      if ( !_boundary->isConcaveSegment( ie1, iSeg2 ))
        break;
    }
    bool vertexFound = false;
    for ( ++iCur; iCur < iNext; ++iCur )
    {
      ie2 = getGeomEdge( maEdgesTwin[ iCur.indexMod() ] );
      if ( ie2 != edgeIDs2.back() )
      {
        // opposite VERTEX found
        divisionPnt._iEdge = iCur.indexMod();
        divisionPnt._edgeParam = 0;
        divPoints.push_back( divisionPnt );
        edgeIDs1.push_back( ie1 );
        edgeIDs2.push_back( ie2 );
        vertexFound = true;
      }
    }
    if ( vertexFound )
    {
      --iNext;
      iPrev = iNext; // not to add a BP in the moddle
      i = iNext.indexMod();
      isConcaveV = true;
    }
  }
  else if ( isConcaPrev )
  {
    // all null-length segments passed, find their beginning
    while ( ( maE = iPrev.edgePrev() ))
    {
      iSeg1 = getBndSegment( maE );
      if ( _boundary->isConcaveSegment( edgeIDs1.back(), iSeg1 ))
        --iPrev;
      else
        break;
    }
  }

  if ( iPrev.index() < i-1 || iNext.index() > i )
  {
    // no VERTEX on the opposite EDGE, put the Branch Point in the middle
    divisionPnt._iEdge = iPrev.indexMod();
    ++iPrev;
    double   par1 = _params[ iPrev.indexMod() ], par2 = _params[ iNext.indexMod() ];
    double midPar = 0.5 * ( par1 + par2 );
    for ( ; _params[ iPrev.indexMod() ] < midPar; ++iPrev )
      divisionPnt._iEdge = iPrev.indexMod();
    divisionPnt._edgeParam =
      ( _params[ iPrev.indexMod() ] - midPar ) /
      ( _params[ iPrev.indexMod() ] - _params[ divisionPnt._iEdge ] );
    divPoints.push_back( divisionPnt );
    isConcaveV = true;
  }

  return isConcaveV;
}

//================================================================================
/*!
 * \brief Return indices of opposite parts of EDGEs equidistant from this branch
 *  \param [out] edgeIDs1 - EDGE index opposite to the edgeIDs2[i]-th EDGE
 *  \param [out] edgeIDs2 - EDGE index opposite to the edgeIDs1[i]-th EDGE
 *  \param [out] divPoints - BranchPoint's located between two successive unique
 *         pairs of EDGE indices. A \a divPoints[i] can separate e.g. two following pairs
 *         of EDGE indices < 0, 2 > and < 0, 1 >. Number of \a divPoints is one less
 *         than number of \a edgeIDs
 */
//================================================================================

void SMESH_MAT2d::Branch::getOppositeGeomEdges( std::vector< std::size_t >& edgeIDs1,
                                                std::vector< std::size_t >& edgeIDs2,
                                                std::vector< BranchPoint >& divPoints) const
{
  edgeIDs1.clear();
  edgeIDs2.clear();
  divPoints.clear();

  std::vector<const TVDEdge*> twins( _maEdges.size() );
  for ( size_t i = 0; i < _maEdges.size(); ++i )
    twins[i] = _maEdges[i]->twin();

  BranchIterator maIter ( _maEdges, 0 );
  BranchIterator twIter ( twins, 0 );
  // size_t lastConcaE1 = _boundary.nbEdges();
  // size_t lastConcaE2 = _boundary.nbEdges();

  // if ( maIter._closed ) // closed branch
  // {
  //   edgeIDs1.push_back( getGeomEdge( _maEdges.back() ));
  //   edgeIDs2.push_back( getGeomEdge( _maEdges.back()->twin() ));
  // }
  // else
  {
    edgeIDs1.push_back( getGeomEdge( maIter.edge() ));
    edgeIDs2.push_back( getGeomEdge( twIter.edge() ));
  }

  BranchPoint divisionPnt;
  divisionPnt._branch = this;

  for ( ++maIter, ++twIter; maIter.index() < _maEdges.size(); ++maIter, ++twIter )
  {
    size_t ie1 = getGeomEdge( maIter.edge() );
    size_t ie2 = getGeomEdge( twIter.edge() );

    bool otherE1 = ( edgeIDs1.back() != ie1 );
    bool otherE2 = ( edgeIDs2.back() != ie2 );

    if ( !otherE1 && !otherE2 && maIter._closed )
    {
      int iSegPrev1 = getBndSegment( maIter.edgePrev() );
      int iSegCur1  = getBndSegment( maIter.edge() );
      otherE1 = Abs( iSegPrev1 - iSegCur1 ) != 1;
      int iSegPrev2 = getBndSegment( twIter.edgePrev() );
      int iSegCur2  = getBndSegment( twIter.edge() );
      otherE2 = Abs( iSegPrev2 - iSegCur2 ) != 1;
    }

    if ( otherE1 || otherE2 )
    {
      bool isConcaveV = false;
      if ( otherE1 && !otherE2 )
      {
        isConcaveV = addDivPntForConcaVertex( edgeIDs1, edgeIDs2, divPoints,
                                              _maEdges, twins, maIter._i );
      }
      if ( !otherE1 && otherE2 )
      {
        isConcaveV = addDivPntForConcaVertex( edgeIDs2, edgeIDs1, divPoints,
                                              twins, _maEdges, maIter._i );
      }

      if ( isConcaveV )
      {
        ie1 = getGeomEdge( maIter.edge() );
        ie2 = getGeomEdge( twIter.edge() );
      }
      if ( !isConcaveV || otherE1 || otherE2 )
      {
        edgeIDs1.push_back( ie1 );
        edgeIDs2.push_back( ie2 );
      }
      if ( divPoints.size() < edgeIDs1.size() - 1 )
      {
        divisionPnt._iEdge = maIter.index();
        divisionPnt._edgeParam = 0;
        divPoints.push_back( divisionPnt );
      }

    } // if ( edgeIDs1.back() != ie1 || edgeIDs2.back() != ie2 )
  } // loop on _maEdges
}

//================================================================================
/*!
 * \brief Store data of boundary segments in TVDEdge
 */
//================================================================================

void SMESH_MAT2d::Branch::setGeomEdge( std::size_t geomIndex, const TVDEdge* maEdge )
{
  if ( maEdge ) maEdge->cell()->color( geomIndex );
}
std::size_t SMESH_MAT2d::Branch::getGeomEdge( const TVDEdge* maEdge )
{
  return maEdge ? maEdge->cell()->color() : std::string::npos;
}
void SMESH_MAT2d::Branch::setBndSegment( std::size_t segIndex, const TVDEdge* maEdge )
{
  if ( maEdge ) maEdge->color( segIndex );
}
std::size_t SMESH_MAT2d::Branch::getBndSegment( const TVDEdge* maEdge )
{
  return maEdge ? maEdge->color() : std::string::npos;
}

//================================================================================
/*!
 * \brief Returns a boundary point on a given EDGE
 *  \param [in] iEdge - index of the EDGE within MedialAxis
 *  \param [in] iSeg - index of a boundary segment within this Branch
 *  \param [in] u - [0;1] normalized param within \a iSeg-th segment
 *  \param [out] bp - the found BoundaryPoint
 *  \return bool - true if the BoundaryPoint is found
 */
//================================================================================

bool SMESH_MAT2d::Boundary::getPoint( std::size_t    iEdge,
                                      std::size_t    iSeg,
                                      double         u,
                                      BoundaryPoint& bp ) const
{
  if ( iEdge >= _pointsPerEdge.size() )
    return false;
  if ( iSeg+1 >= _pointsPerEdge[ iEdge ]._params.size() )
    return false;

  // This method is called by Branch that can have an opposite orientation,
  // hence u is inverted depending on orientation coded as a sign of _maEdge index
  bool isReverse = ( _pointsPerEdge[ iEdge ]._maEdges[ iSeg ].second < 0 );
  if ( isReverse )
    u = 1. - u;

  double p0 = _pointsPerEdge[ iEdge ]._params[ iSeg ];
  double p1 = _pointsPerEdge[ iEdge ]._params[ iSeg+1 ];

  bp._param = p0 * ( 1. - u ) + p1 * u;
  bp._edgeIndex = iEdge;

  return true;
}

