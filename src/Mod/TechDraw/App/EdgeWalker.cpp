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


#include "PreCompiled.h"

#include <Base/Console.h>

#include "EdgeWalker.h"

using namespace TechDraw;
using namespace boost;

//*******************************************************
//* edgeVisior methods
//*******************************************************
template <typename Edge>
void edgeVisitor::next_edge(Edge e)
{
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
    faceEdges.clear();
}

void edgeVisitor::end_face()
{
    graphFaces.push_back(faceEdges);
}

TechDraw::facelist edgeVisitor::getResult(void)
{
    return graphFaces;
}

void edgeVisitor::setGraph(TechDraw::graph& g)
{
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

bool EdgeWalker::loadEdges(std::vector<TechDraw::WalkerEdge> edges)
{
    for (auto e: edges) {
        add_edge(e.v1,e.v2,m_g);
    }
    return true;
}

bool EdgeWalker::setSize(int size)
{
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
   return result;
}

//static methods
bool EdgeWalker::orderEdges(WalkerEdge i, WalkerEdge j)
{
    return (i.idx < j.idx);
}

bool EdgeWalker::isEqual(edgelist el1, edgelist el2)
{
    bool result = true;
    if (el1.size() != el2.size()) {
        result = false;
    } else {
        std::sort(el1.begin(),el1.end(),orderEdges);
        std::sort(el2.begin(),el2.end(),orderEdges);
        for (unsigned int i = 0; i < el1.size(); i ++) {
            if (el1.at(i).idx != el2.at(i).idx) {
                result = false;
                break;
            }
        }
    }
    return result;
}

//check faces that use the same set of edges, but maybe in a different order.
facelist EdgeWalker::removeDuplicateFaces(facelist in)
{
    facelist result;
    result.push_back(*(in.begin()));                //save the first edgelist
    facelist::iterator iFace = (in.begin()) + 1;    //starting with second
    for (; iFace != in.end(); iFace++) {
        bool addToResult = true;
        for (auto& e:result) {
            if (isEqual((*iFace),e))  {             //already in result?
                addToResult = false;
                break;
            }
        }
        if (addToResult) {
            result.push_back((*iFace));
        }
    }
    return result;
}
