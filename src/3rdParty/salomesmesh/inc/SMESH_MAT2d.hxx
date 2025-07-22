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
// File      : SMESH_MAT2d.hxx
// Created   : Thu May 28 17:49:53 2015
// Author    : Edward AGAPOV (eap)

#ifndef __SMESH_MAT2d_HXX__
#define __SMESH_MAT2d_HXX__

#include "SMESH_Utils.hxx"

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <gp.hxx> //added for occ6

#include <vector>
#include <map>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>

class Adaptor3d_Curve;

// Medial Axis Transform 2D
namespace SMESH_MAT2d
{
  class MedialAxis; // MedialAxis is the entry point
  class Branch;
  struct BranchEnd;
  class Boundary;
  struct BoundaryPoint;

  typedef boost::polygon::voronoi_diagram<double> TVD;
  typedef TVD::cell_type                          TVDCell;
  typedef TVD::edge_type                          TVDEdge;
  typedef TVD::vertex_type                        TVDVertex;

  //-------------------------------------------------------------------------------------
  // type of Branch end point
  enum BranchEndType { BE_UNDEF,
                       BE_ON_VERTEX, // branch ends at a convex VRTEX
                       BE_BRANCH_POINT, // branch meats 2 or more other branches
                       BE_END // branch end equidistant from several adjacent segments
  };
  //-------------------------------------------------------------------------------------
  /*!
   * \brief End point of MA Branch
   */
  struct SMESHUtils_EXPORT BranchEnd
  {
    const TVDVertex*             _vertex;
    BranchEndType                _type;
    std::vector< const Branch* > _branches;

    BranchEnd(): _vertex(0), _type( BE_UNDEF ) {}
  };
  //-------------------------------------------------------------------------------------
  /*!
   * \brief Point on MA Branch
   */
  struct SMESHUtils_EXPORT BranchPoint
  {
    const Branch* _branch;
    std::size_t   _iEdge; // MA edge index within the branch
    double        _edgeParam; // normalized param within the MA edge

    BranchPoint( const Branch* b = 0, std::size_t e = 0, double u = -1 ):
      _branch(b), _iEdge(e), _edgeParam(u) {}
  };
  //-------------------------------------------------------------------------------------
  /*!
   * \brief Branch is a set of MA edges enclosed between branch points and/or MA ends.
   *        It's main feature is to return two BoundaryPoint's per a point on it.
   *        Points on a Branch are defined by [0,1] parameter 
   */
  class SMESHUtils_EXPORT Branch
  {
  public:
    bool getBoundaryPoints(double param, BoundaryPoint& bp1, BoundaryPoint& bp2 ) const;
    bool getBoundaryPoints(std::size_t iMAEdge, double maEdgeParam,
                           BoundaryPoint& bp1, BoundaryPoint& bp2 ) const;
    bool getBoundaryPoints(const BranchPoint& p,
                           BoundaryPoint& bp1, BoundaryPoint& bp2 ) const;
    bool getParameter(const BranchPoint& p, double & u ) const;

    std::size_t      nbEdges() const { return _maEdges.size(); }

    const BranchEnd* getEnd(bool the2nd) const { return & ( the2nd ? _endPoint2 : _endPoint1 ); }

    bool hasEndOfType(BranchEndType type) const;

    void getPoints( std::vector< gp_XY >& points, const double scale[2]) const;

    void getGeomEdges( std::vector< std::size_t >& edgeIDs1,
                       std::vector< std::size_t >& edgeIDs2 ) const;

    void getOppositeGeomEdges( std::vector< std::size_t >& edgeIDs1,
                               std::vector< std::size_t >& edgeIDs2,
                               std::vector< BranchPoint >& divPoints) const;

    bool isRemoved() const { return _proxyPoint._branch; }

  public: // internal: construction

    void init( std::vector<const TVDEdge*>&                maEdges,
               const Boundary*                             boundary,
               std::map< const TVDVertex*, BranchEndType > endType);
    void setBranchesToEnds( const std::vector< Branch >&   branches);
    BranchPoint getPoint( const TVDVertex* vertex ) const;
    void setRemoved( const BranchPoint& proxyPoint );

    static void        setGeomEdge  ( std::size_t geomIndex, const TVDEdge* maEdge );
    static std::size_t getGeomEdge  ( const TVDEdge* maEdge );
    static void        setBndSegment( std::size_t segIndex, const TVDEdge* maEdge );
    static std::size_t getBndSegment( const TVDEdge* maEdge );

  private:

    bool addDivPntForConcaVertex( std::vector< std::size_t >&        edgeIDs1,
                                  std::vector< std::size_t >&        edgeIDs2,
                                  std::vector< BranchPoint >&        divPoints,
                                  const std::vector<const TVDEdge*>& maEdges,
                                  const std::vector<const TVDEdge*>& maEdgesTwin,
                                  int &                              i) const;

    // association of _maEdges with boundary segments is stored in this way:
    // index of an EDGE:           TVDEdge->cell()->color()
    // index of a segment on EDGE: TVDEdge->color()
    std::vector<const TVDEdge*> _maEdges; // MA edges ending at points located at _params
    std::vector<double>  _params; // params of points on MA, normalized [0;1] within this branch
    const Boundary*      _boundary; // face boundary
    BranchEnd            _endPoint1;
    BranchEnd            _endPoint2;
    BranchPoint          _proxyPoint;
  };

  //-------------------------------------------------------------------------------------
  /*!
   * \brief Data of a discretized EDGE allowing to get a point on MA by a parameter on EDGE
   */
  struct BndPoints
  {
    std::vector< double > _params; // params of discretization points on an EDGE
    std::vector< std::pair< const Branch*, int > > _maEdges; /* index of TVDEdge in branch;
                                                                index sign means orientation;
                                                                index == Branch->nbEdges() means
                                                                end point of a Branch */
  };
  //-------------------------------------------------------------------------------------
  /*!
   * \brief Face boundary is discretized so that each its segment to correspond to
   *        an edge of MA
   */
  class SMESHUtils_EXPORT Boundary
  {
  public:

    Boundary( std::size_t nbEdges ): _pointsPerEdge( nbEdges ) {}
    BndPoints&  getPoints( std::size_t iEdge ) { return _pointsPerEdge[ iEdge ]; }
    std::size_t nbEdges() const { return _pointsPerEdge.size(); }

    bool getPoint( std::size_t iEdge, std::size_t iSeg, double u, BoundaryPoint& bp ) const;

    bool getBranchPoint( const std::size_t iEdge, double u, BranchPoint& p ) const;

    bool getBranchPoint( const BoundaryPoint& bp, BranchPoint& p ) const;

    bool isConcaveSegment( std::size_t iEdge, std::size_t iSeg ) const;

    bool moveToClosestEdgeEnd( BoundaryPoint& bp ) const;

  private:
    std::vector< BndPoints > _pointsPerEdge;
  };

  //-------------------------------------------------------------------------------------
  /*!
   * \brief Point on FACE boundary
   */
  struct SMESHUtils_EXPORT BoundaryPoint
  {
    std::size_t _edgeIndex; // index of an EDGE in a sequence passed to MedialAxis()
    double      _param;     // parameter of this EDGE
  };
  //-------------------------------------------------------------------------------------
  /*!
   * \brief Medial axis (MA) is defined as the loci of centres of locally
   *        maximal balls inside 2D representation of a face. This class
   *        implements a piecewise approximation of MA.
   */
  class SMESHUtils_EXPORT MedialAxis
  {
  public:
    MedialAxis(const TopoDS_Face&                face,
               const std::vector< TopoDS_Edge >& edges,
               const double                      minSegLen,
               const bool                        ignoreCorners = false );
    std::size_t                            nbBranches() const { return _nbBranches; }
    const Branch*                          getBranch(size_t i) const;
    const std::vector< const BranchEnd* >& getBranchPoints() const { return _branchPnt; }
    const Boundary&                        getBoundary() const { return _boundary; }

    void getPoints( const Branch* branch, std::vector< gp_XY >& points) const;
    Adaptor3d_Curve* make3DCurve(const Branch& branch) const;

  private:

  private:
    TopoDS_Face                     _face;
    TVD                             _vd;
    std::vector< Branch >           _branch;
    std::size_t                     _nbBranches; // removed branches ignored
    std::vector< const BranchEnd* > _branchPnt;
    Boundary                        _boundary;
    double                          _scale[2];
  };

}

#endif
