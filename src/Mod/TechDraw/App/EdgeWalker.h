/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/property_map/property_map.hpp>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw {
//using namespace boost;

using graph =
    boost::adjacency_list
        < boost::vecS,
          boost::vecS,
          boost::bidirectionalS,
          boost::property<boost::vertex_index_t, int>,
          boost::property<boost::edge_index_t, int>
        >;

using vertex_t =
    boost::graph_traits < graph >::vertex_descriptor;

using edge_t =
    boost::graph_traits < graph >::edge_descriptor;

using planar_embedding_storage_t =
    std::vector< std::vector<edge_t> >;

using planar_embedding_t =
    boost::iterator_property_map< planar_embedding_storage_t::iterator,
                                  boost::property_map<graph, boost::vertex_index_t>::type
                                >;

class TechDrawExport WalkerEdge
{
public:
    WalkerEdge();
    static bool weCompare(WalkerEdge i, WalkerEdge j);
    bool isEqual(WalkerEdge w);
    std::string dump();

    std::size_t v1 {0};
    std::size_t v2 {0};
    edge_t ed;
    std::size_t idx {0};
};

class TechDrawExport ewWire
{
public:
    bool isEqual(ewWire w);

    std::vector<WalkerEdge>  wedges;      //[WE] representing 1 wire
    void push_back(WalkerEdge w);
    void clear() {wedges.clear();}
    std::size_t size(void);
};

class TechDrawExport ewWireList
{
public:
    ewWireList removeDuplicateWires();

    std::vector<ewWire> wires;
    void push_back(ewWire e);
    std::size_t size(void);
};



class TechDrawExport edgeVisitor : public boost::planar_face_traversal_visitor
{
public:
    template <typename Edge>
    void next_edge(Edge e);
    void begin_face();
    void end_face();
    ewWireList getResult();     //a list of many wires
    void setGraph(graph& g);

private:
    ewWire wireEdges;
    ewWireList graphWires;
    TechDraw::graph m_g;
};

class TechDrawExport incidenceItem
{
public:
    incidenceItem() {iEdge = 0; angle = 0.0;}
    incidenceItem(std::size_t idx, double a, edge_t ed)  {iEdge = idx; angle = a; eDesc = ed;}
    ~incidenceItem()  = default;
    static bool iiCompare(const incidenceItem& i1, const incidenceItem& i2);
    static bool iiEqual(const incidenceItem& i1, const incidenceItem& i2);
    std::size_t iEdge;
    double angle;
    edge_t eDesc;
};

class TechDrawExport embedItem
{
public:
    embedItem();
    embedItem(std::size_t i,
              std::vector<incidenceItem> list) { iVertex = i; incidenceList = list;}
    ~embedItem()  = default;

    std::size_t iVertex;
    std::vector<incidenceItem> incidenceList;
    std::string dump();
    static std::vector<incidenceItem> sortIncidenceList (std::vector<incidenceItem> &list, bool ascend);
};


class TechDrawExport EdgeWalker
{
public:
    EdgeWalker();
    virtual ~EdgeWalker();

    bool loadEdges(std::vector<TechDraw::WalkerEdge>& edges);
    bool loadEdges(std::vector<TopoDS_Edge> edges);
    bool setSize(std::size_t size);
    std::vector<TopoDS_Wire> execute(std::vector<TopoDS_Edge> edgeList, bool biggie = true);

    ewWireList getResult();
    std::vector<TopoDS_Wire> getResultWires();
    std::vector<TopoDS_Wire> getResultNoDups();

    std::vector<TopoDS_Vertex> makeUniqueVList(std::vector<TopoDS_Edge> edges);
    std::vector<WalkerEdge>    makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                               std::vector<TopoDS_Vertex> verts);

    size_t findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert);
    std::vector<TopoDS_Wire> sortStrip(std::vector<TopoDS_Wire> fw, bool includeBiggest);
    std::vector<TopoDS_Wire> sortWiresBySize(std::vector<TopoDS_Wire>& w, bool reverse = false);
    static TopoDS_Wire makeCleanWire(std::vector<TopoDS_Edge> edges, double tol = 0.10);

    std::vector<int> getEmbeddingRowIx(int v);
    std::vector<edge_t> getEmbeddingRow(int v);
    std::vector<embedItem> makeEmbedding(const std::vector<TopoDS_Edge> edges,
                                                 const std::vector<TopoDS_Vertex> uniqueVList);

protected:
    bool prepare();
    static bool wireCompare(const TopoDS_Wire& w1, const TopoDS_Wire& w2);
    std::vector<TechDraw::WalkerEdge> m_saveWalkerEdges;
    std::vector<TopoDS_Edge> m_saveInEdges;
    std::vector<embedItem> m_embedding;

private:
    edgeVisitor m_eV;
    TechDraw::graph m_g;
};

}  //end namespace TechDraw