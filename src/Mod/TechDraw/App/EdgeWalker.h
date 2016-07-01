/***************************************************************************
 *   Copyright (c) 2016 Wandererfan <wandererfan@gmail.com>                *
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
/*
 * Some material based on Boost sample code                                */
// Distributed under the Boost Software License, Version 1.0. (See
// http://www.boost.org/LICENSE_1_0.txt)
//**************************************************************************

#ifndef TECHDRAW_EDGEWALKER_H
#define TECHDRAW_EDGEWALKER_H

#include <vector>
#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/ref.hpp>

namespace TechDraw {
using namespace boost;

typedef adjacency_list
    < vecS,
      vecS,
      undirectedS,
      property<vertex_index_t, int>,
      property<edge_index_t, int>
    >
    graph;

struct WalkerEdge {
    std::size_t v1;
    std::size_t v2;
    int idx;
};

typedef std::vector<WalkerEdge>  edgelist;
typedef std::vector<edgelist> facelist ;


class edgeVisitor : public planar_face_traversal_visitor
{
public:
  template <typename Edge>
  void next_edge(Edge e);
  void begin_face();
  void end_face();
  facelist getResult(void);
  void setGraph(graph& g);

private:
    TechDraw::edgelist faceEdges;
    TechDraw::facelist graphFaces;
    TechDraw::graph m_g;
};

class EdgeWalker
{
public:
    EdgeWalker(void);
    virtual ~EdgeWalker();

    bool loadEdges(std::vector<TechDraw::WalkerEdge> edges);
    bool setSize(int size);
    bool perform();
    facelist getResult();

private:
    edgeVisitor m_eV;
    TechDraw::graph m_g;
};

}  //end namespace

#endif //TECHDRAW_EDGEWALKER_H
