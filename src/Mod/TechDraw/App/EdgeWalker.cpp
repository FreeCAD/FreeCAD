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
/*                                                                         *
 * Some material based on Boost sample code                                */
// Distributed under the Boost Software License, Version 1.0. (See
// http://www.boost.org/LICENSE_1_0.txt)
//**************************************************************************


//#include "PreCompiled.h"

#include "EdgeWalker.h"

using namespace TechDraw;
using namespace boost;

//*******************************************************
//* edgeVisior methods
//*******************************************************
template <typename Edge>
void edgeVisitor::next_edge(Edge e)
{
    std::cout << e << " ";
    graph_traits<graph>::vertex_descriptor s = source(e,m_g);
    graph_traits<graph>::vertex_descriptor t = target(e,m_g);
    WalkerEdge we;
    we.v1 = s;
    we.v2 = t;
    we.idx = get(edge_index,m_g,e);
    faceEdges.push_back(we);
}

void edgeVisitor::begin_face()
{
    std::cout << "begin_face()" << std::endl;
    faceEdges.clear();
}

void edgeVisitor::end_face()
{
    std::cout << "end_face()" << std::endl;
    graphFaces.push_back(faceEdges);
}

facelist edgeVisitor::getResult(void)
{
    return graphFaces;
}

void edgeVisitor::setGraph(graph& g)
{
    std::cout << "setGraph()" << std::endl;
    m_g = g;
}

//*******************************************************
//* EdgeWalker
//*******************************************************

EdgeWalker::EdgeWalker()
{
}

EdgeWalker::~EdgeWalker()
{
}

bool EdgeWalker::loadEdges(std::vector<WalkerEdge> edges)
{
    std::cout << "loadEdges()" << std::endl;
    for (auto e: edges) {
        add_edge(e.v1,e.v2,m_g);
    }
//    add_edge(0, 1, m_g);
//    add_edge(2, 3, m_g);
//    add_edge(4, 0, m_g);
//    add_edge(5, 3, m_g);
//    add_edge(6, 7, m_g);
//    add_edge(6, 4, m_g);
//    add_edge(7, 5, m_g);
//    add_edge(8, 9, m_g);
//    add_edge(4, 1, m_g);
//    add_edge(1, 8, m_g);
//    add_edge(5, 2, m_g);
//    add_edge(2, 9, m_g);
    return true;
}

bool EdgeWalker::setSize(int size)
{
    std::cout << "setsize()" << std::endl;
    m_g.clear();
    for (int i = 0; i < size; i++) {
        boost::adjacency_list<>::vertex_descriptor vd = boost::add_vertex(m_g);
    }
    return true;
}

bool EdgeWalker::perform()
{
    // Initialize the interior edge index
    //property<edge_index_t, int>
    property_map<TechDraw::graph, edge_index_t>::type e_index = get(edge_index, m_g);
    graph_traits<TechDraw::graph>::edges_size_type edge_count = 0;
    graph_traits<TechDraw::graph>::edge_iterator ei, ei_end;
    for(boost::tie(ei, ei_end) = edges(m_g); ei != ei_end; ++ei)
      put(e_index, *ei, edge_count++);

    // Test for planarity - we know it is planar, we just want to
    // compute the planar embedding as a side-effect
    typedef std::vector< graph_traits<TechDraw::graph>::edge_descriptor > vec_t;
    std::vector<vec_t> embedding(num_vertices(m_g));
    boyer_myrvold_planarity_test(boyer_myrvold_params::graph = m_g,
                                 boyer_myrvold_params::embedding = &embedding[0]);

    m_eV.setGraph(m_g);
    planar_face_traversal(m_g, &embedding[0], m_eV);

    return true;
}

facelist EdgeWalker::getResult()
{
  TechDraw::facelist result = m_eV.getResult();
  TechDraw::facelist::iterator iFace = result.begin();
  for (;iFace != result.end(); iFace++) {
      std::cout << "face begins:" <<  std::endl;
      TechDraw::edgelist::iterator iEdge = (*iFace).begin();
      for (;iEdge != (*iFace).end(); iEdge++) {
          std::cout << (*iEdge).idx << ":(" << (*iEdge).v1 << ", " << (*iEdge).v2 << ") ";
      }
      std::cout << std::endl;
   }
   return result;
}
