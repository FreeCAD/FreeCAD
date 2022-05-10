/***************************************************************************
 *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
#ifndef PATH_VORONOI_H
#define PATH_VORONOI_H

#include <climits>
#include <map>
#include <vector>
#include <Base/BaseClass.h>
#include <Base/Handle.h>
#include <Base/Vector3D.h>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>

#if (SIZE_MAX == UINT_MAX)
# define PATH_VORONOI_COLOR_MASK 0x07FFFFFFul
#else
# define PATH_VORONOI_COLOR_MASK 0x07FFFFFFFFFFFFFFul
#endif

namespace Path
{
  class PathExport Voronoi
    : public Base::BaseClass
  {
    TYPESYSTEM_HEADER();

  public:
    //constructors
    Voronoi();
    ~Voronoi();

    typedef std::size_t   color_type;
    static const int        InvalidIndex = INT_MAX;
    static const color_type ColorMask    = PATH_VORONOI_COLOR_MASK;

    // types
    typedef double coordinate_type;
    typedef boost::polygon::voronoi_vertex<double> vertex_type;
    typedef boost::polygon::point_data<coordinate_type> point_type;
    typedef boost::polygon::segment_data<coordinate_type> segment_type;
    typedef boost::polygon::voronoi_diagram<double> voronoi_diagram_type;

    class diagram_type
      : public voronoi_diagram_type
      , public Base::Handled
    {
    public:
      diagram_type();

      double getScale() const;
      void   setScale(double s);

      Base::Vector3d scaledVector(double x, double y, double z) const;
      Base::Vector3d scaledVector(const point_type &p, double z) const;
      Base::Vector3d scaledVector(const vertex_type &v, double z) const;

      typedef std::map<intptr_t, int> cell_map_type;
      typedef std::map<intptr_t, int> edge_map_type;
      typedef std::map<intptr_t, int> vertex_map_type;

      int index(const cell_type   *cell)   const;
      int index(const edge_type   *edge)   const;
      int index(const vertex_type *vertex) const;

      void reIndex();

      std::vector<point_type>       points;
      std::vector<segment_type>     segments;

      point_type    retrievePoint(const cell_type *cell) const;
      segment_type  retrieveSegment(const cell_type *cell) const;

      typedef std::map<int, double> angle_map_t;
      double angleOfSegment(int i, angle_map_t *angle = nullptr) const;
      bool segmentsAreConnected(int i, int j) const;

    private:
      double          scale;
      cell_map_type   cell_index;
      edge_map_type   edge_index;
      vertex_map_type vertex_index;
    };

    void addPoint(const point_type &p);
    void addSegment(const segment_type &p);
    long numPoints() const;
    long numSegments() const;

    void construct();
    long numCells() const;
    long numEdges() const;
    long numVertices() const;

    void resetColor(color_type color);
    void colorExterior(color_type color);
    void colorTwins(color_type color);
    void colorColinear(color_type color, double degree);

    template<typename T>
    T* create(int index) {
      return new T(vd, index);
    }

    double getScale() const { return vd->getScale(); }
    void   setScale(double scale) { vd->setScale(scale); }

  private:
    Base::Reference<diagram_type> vd;
    friend class VoronoiPy;
    void colorExterior(const Voronoi::diagram_type::edge_type *edge, std::size_t colorValue);
  };

} //namespace Path

#endif // PATH_VORONOI_H
