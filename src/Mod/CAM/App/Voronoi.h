// SPDX-License-Identifier: LGPL-2.1-or-later
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
#pragma once

#include <limits>
#include <map>
#include <vector>
#include <Base/BaseClass.h>
#include <Base/Handle.h>
#include <Base/Vector3D.h>
#include <Mod/CAM/PathGlobal.h>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>


namespace Path
{
class PathExport Voronoi: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    // constructors
    Voronoi();
    ~Voronoi() override;

    using color_type = std::size_t;
    static const int InvalidIndex = std::numeric_limits<int>::max();
    static const color_type ColorMask = std::numeric_limits<color_type>::max() >> 5;

    // types
    using coordinate_type = double;
    using vertex_type = boost::polygon::voronoi_vertex<double>;
    using point_type = boost::polygon::point_data<coordinate_type>;
    using segment_type = boost::polygon::segment_data<coordinate_type>;
    using voronoi_diagram_type = boost::polygon::voronoi_diagram<double>;

    class diagram_type: public voronoi_diagram_type, public Base::Handled
    {
    public:
        diagram_type();

        double getScale() const;
        void setScale(double s);

        Base::Vector3d scaledVector(double x, double y, double z) const;
        Base::Vector3d scaledVector(const point_type& p, double z) const;
        Base::Vector3d scaledVector(const vertex_type& v, double z) const;

        using cell_map_type = std::map<intptr_t, int>;
        using edge_map_type = std::map<intptr_t, int>;
        using vertex_map_type = std::map<intptr_t, int>;

        int index(const cell_type* cell) const;
        int index(const edge_type* edge) const;
        int index(const vertex_type* vertex) const;

        void reIndex();

        std::vector<point_type> points;
        std::vector<segment_type> segments;

        point_type retrievePoint(const cell_type* cell) const;
        segment_type retrieveSegment(const cell_type* cell) const;

        using angle_map_t = std::map<int, double>;
        double angleOfSegment(int i, angle_map_t* angle = nullptr) const;
        bool segmentsAreConnected(int i, int j) const;

    private:
        double scale;
        cell_map_type cell_index;
        edge_map_type edge_index;
        vertex_map_type vertex_index;
    };

    void addPoint(const point_type& p);
    void addSegment(const segment_type& p);
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
    T* create(int index)
    {
        return new T(vd, index);
    }

    double getScale() const
    {
        return vd->getScale();
    }
    void setScale(double scale)
    {
        vd->setScale(scale);
    }

private:
    Base::Reference<diagram_type> vd;
    friend class VoronoiPy;
    void colorExterior(const Voronoi::diagram_type::edge_type* edge, std::size_t colorValue);
};

}  // namespace Path
