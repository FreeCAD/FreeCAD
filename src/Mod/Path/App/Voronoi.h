/***************************************************************************
 *   Copyright (c) sliptonic (shopinthewoods@gmail.com) 2020               *
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

#include <map>
#include <string>
#include <Base/BaseClass.h>
#include <Base/Handle.h>
#include <Base/Vector3D.h>

#include <vector>
#include <boost/polygon/point_concept.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/polygon/segment_concept.hpp>
#include <boost/polygon/voronoi.hpp>

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

    static const int InvalidIndex = INT_MAX;
    // types
    typedef double coordinate_type;
    typedef boost::polygon::point_data<coordinate_type> point_type;
    typedef boost::polygon::segment_data<coordinate_type> segment_type;
    typedef boost::polygon::voronoi_diagram<coordinate_type> voronoi_diagram_type;

    class diagram_type
      : public voronoi_diagram_type
      , public Base::Handled
    {
    public:

      typedef std::map<intptr_t, int> cell_map_type;
      typedef std::map<intptr_t, int> edge_map_type;
      typedef std::map<intptr_t, int> vertex_map_type;

      int index(const cell_type   *cell)   const;
      int index(const edge_type   *edge)   const;
      int index(const vertex_type *vertex) const;

      void reIndex();

      std::vector<point_type>       points;
      std::vector<segment_type>     segments;

    private:
      cell_map_type   cell_index;
      edge_map_type   edge_index;
      vertex_map_type vertex_index;
    };

    // specific methods
    void addPoint(const point_type &p);
    void addSegment(const segment_type &p);

    void construct();
    long numCells() const;
    long numEdges() const;
    long numVertices() const;

    // attributes
    Base::Reference<diagram_type> vd;
  };

} //namespace Path

#endif // PATH_VORONOI_H
