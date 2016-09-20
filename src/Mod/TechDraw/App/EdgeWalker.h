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
#include <boost/graph/is_kuratowski_subgraph.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/ref.hpp>

#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

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

class WalkerEdge
{
public:
    static bool weCompare(WalkerEdge i, WalkerEdge j);
    bool isEqual(WalkerEdge w);

    std::size_t v1;
    std::size_t v2;
    int idx;
};

class ewWire
{
public:
    bool isEqual(ewWire w);

    std::vector<WalkerEdge>  wedges;      //[WE] representing 1 wire
    void push_back(WalkerEdge w);
    void clear() {wedges.clear();};
};

class ewWireList
{
public:
    ewWireList removeDuplicateWires();

    std::vector<ewWire> wires;
    void push_back(ewWire e);
};



class edgeVisitor : public planar_face_traversal_visitor
{
public:
    template <typename Edge>
    void next_edge(Edge e);
    void begin_face();
    void end_face();
    ewWireList getResult(void);     //a list of many wires
    void setGraph(graph& g);

private:
    ewWire wireEdges;
    ewWireList graphWires;
    TechDraw::graph m_g;
};

class EdgeWalker
{
public:
    EdgeWalker(void);
    virtual ~EdgeWalker();

    bool loadEdges(std::vector<TechDraw::WalkerEdge> edges);
    bool loadEdges(std::vector<TopoDS_Edge> edges);
    bool setSize(int size);
    bool perform();
    ewWireList getResult();
    std::vector<TopoDS_Wire> getResultWires();
    std::vector<TopoDS_Wire> getResultNoDups();

    std::vector<TopoDS_Vertex> makeUniqueVList(std::vector<TopoDS_Edge> edges);
    std::vector<WalkerEdge>    makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                               std::vector<TopoDS_Vertex> verts);

    int findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert);
    std::vector<TopoDS_Wire> sortStrip(std::vector<TopoDS_Wire> fw, bool includeBiggest);
    std::vector<TopoDS_Wire> sortWiresBySize(std::vector<TopoDS_Wire>& w, bool reverse = false);
    TopoDS_Wire makeCleanWire(std::vector<TopoDS_Edge> edges, double tol = 0.10);


protected:
    static bool wireCompare(const TopoDS_Wire& w1, const TopoDS_Wire& w2);
    std::vector<TopoDS_Edge> saveInEdges;

private:
    edgeVisitor m_eV;
    TechDraw::graph m_g;
};

}  //end namespace TechDraw

#endif //TECHDRAW_EDGEWALKER_H
