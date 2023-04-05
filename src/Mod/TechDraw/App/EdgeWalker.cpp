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
/*                                                                         *
 * Some material based on Boost sample code                                */
// Distributed under the Boost Software License, Version 1.0. (See
// http://www.boost.org/LICENSE_1_0.txt)
//**************************************************************************

#include "PreCompiled.h"

#ifndef _PreComp_
# include <cmath>
# include <sstream>
# include <BRep_Tool.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <ShapeExtend_WireData.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopExp.hxx>
# include <boost/graph/boyer_myrvold_planar_test.hpp>
#endif

#include <Base/Console.h>

#include "EdgeWalker.h"
#include "DrawUtil.h"


using namespace TechDraw;
using namespace boost;

//*******************************************************
//* edgeVisior methods
//*******************************************************
template <typename Edge>
void edgeVisitor::next_edge(Edge e)
{
    graph_traits<graph>::vertex_descriptor s = source(e, m_g);
    graph_traits<graph>::vertex_descriptor t = target(e, m_g);
    WalkerEdge we;
    we.v1 = s;
    we.v2 = t;
    we.ed = e;
    we.idx = get(edge_index, m_g, e);
    wireEdges.push_back(we);
}

void edgeVisitor::begin_face()
{
//    Base::Console().Message("EV::begin_face()\n");
    wireEdges.clear();
}

void edgeVisitor::end_face()
{
//    Base::Console().Message("EV::end_face()\n");
    graphWires.push_back(wireEdges);
}

TechDraw::ewWireList edgeVisitor::getResult()
{
    return graphWires;
}

void edgeVisitor::setGraph(TechDraw::graph& g)
{
    m_g = g;
}

//*******************************************************
//* EdgeWalker methods
//*******************************************************

EdgeWalker::EdgeWalker()
{
}

EdgeWalker::~EdgeWalker()
{
}

//loads a list of unique edges into the traversal mechanism
bool EdgeWalker::loadEdges(std::vector<TechDraw::WalkerEdge>& edges)
{
//    Base::Console().Message("EW::loadEdges(we) - WEdgesIn: %d\n", edges.size());
    int idx = 0;
    for (auto& e: edges) {
        std::pair<edge_t, bool> p;
        p = add_edge(e.v1, e.v2, m_g);
        e.ed = p.first;
        e.idx = idx;
        idx++;
        m_saveWalkerEdges.push_back(e);
    }

    return true;
}

bool EdgeWalker::loadEdges(std::vector<TopoDS_Edge> edges)
{
//    Base::Console().Message("EW::loadEdges(TopoDS) - edges: %d\n", edges.size());
    if (edges.empty()) {
        throw Base::ValueError("EdgeWalker has no edges to load\n");
    }

    std::vector<TopoDS_Vertex> verts = makeUniqueVList(edges);
    setSize(verts.size());

    std::vector<WalkerEdge>  we  = makeWalkerEdges(edges, verts);
    loadEdges(we);

    m_embedding = makeEmbedding(edges, verts);

    return true;
}
bool EdgeWalker::setSize(std::size_t size)
{
    m_g.clear();
    for (std::size_t i = 0; i < size; i++) {
        boost::adjacency_list<>::vertex_descriptor vd = boost::add_vertex(m_g);
        (void)vd;
    }
    return true;
}

bool EdgeWalker::prepare()
{
    //Base::Console().Message("TRACE - EW::prepare()\n");
    // Initialize the interior edge index
    property_map<TechDraw::graph, edge_index_t>::type e_index = get(edge_index, m_g);
    graph_traits<TechDraw::graph>::edges_size_type edge_count = 0;
    graph_traits<TechDraw::graph>::edge_iterator ei, ei_end;
    for(boost::tie(ei, ei_end) = edges(m_g); ei != ei_end; ++ei)
      put(e_index, *ei, edge_count++);

    //make planar embedding
    planar_embedding_storage_t planar_embedding_storage(num_vertices(m_g));
    planar_embedding_t planar_embedding(planar_embedding_storage.begin(),
                                    get(vertex_index, m_g) );
    for (auto& e : m_embedding) {
        for (auto&  i: e.incidenceList) {
            planar_embedding[e.iVertex].push_back(i.eDesc);
        }
    }

    m_eV.setGraph(m_g);
    planar_face_traversal(m_g, &planar_embedding[0], m_eV);

    return true;
}

std::vector<TopoDS_Wire> EdgeWalker::execute(std::vector<TopoDS_Edge> edgeList, bool biggie)
{
    loadEdges(edgeList);
    bool success = prepare();
    if (success) {
        std::vector<TopoDS_Wire> rw = getResultNoDups();
        return sortStrip(rw, biggie);
    }
    return std::vector<TopoDS_Wire>();
}

ewWireList EdgeWalker::getResult()
{
    //Base::Console().Message("TRACE - EW::getResult()\n");
    // Return value is a list of many wires each of which is a list of many WE
    return m_eV.getResult();
}

std::vector<TopoDS_Wire> EdgeWalker::getResultWires()
{
    //Base::Console().Message("TRACE - EW::getResultWires()\n");
    std::vector<TopoDS_Wire> fw;
    ewWireList result = m_eV.getResult();
    if (result.wires.empty()) {
        return fw;
    }

    std::vector<ewWire>::iterator iWire = result.wires.begin();     // a WE within [WE]
    for (;iWire != result.wires.end(); iWire++) {
        std::vector<WalkerEdge>::iterator iEdge = (*iWire).wedges.begin();
        std::vector<TopoDS_Edge> topoEdges;
        for (;iEdge != (*iWire).wedges.end(); iEdge++) {
            TopoDS_Edge e = m_saveInEdges.at((*iEdge).idx);
            topoEdges.push_back(e);
        }
    TopoDS_Wire w = makeCleanWire(topoEdges, EWTOLERANCE);             //make 1 clean wire from its edges
    fw.push_back(w);
    }
    return fw;
}

std::vector<TopoDS_Wire> EdgeWalker::getResultNoDups()
{
    //Base::Console().Message("TRACE - EW::getResultNoDups()\n");
    std::vector<TopoDS_Wire> fw;
    ewWireList result = m_eV.getResult();
    if (result.wires.empty()) {
        return fw;
    }

    result = result.removeDuplicateWires();

    std::vector<ewWire>::iterator iWire = result.wires.begin();
    for (;iWire != result.wires.end(); iWire++) {
        std::vector<WalkerEdge>::iterator iEdge = (*iWire).wedges.begin();
        std::vector<TopoDS_Edge> topoEdges;
        for (;iEdge != (*iWire).wedges.end(); iEdge++) {
            TopoDS_Edge e = m_saveInEdges.at((*iEdge).idx);
            topoEdges.push_back(e);
        }
        TopoDS_Wire w = makeCleanWire(topoEdges, EWTOLERANCE);             //make 1 clean wire from its edges
        fw.push_back(w);
    }
    return fw;
}

//* static *//
//! make a clean wire with sorted, oriented, connected, etc edges
TopoDS_Wire EdgeWalker::makeCleanWire(std::vector<TopoDS_Edge> edges, double tol)
{
    //Base::Console().Message("TRACE - EW::makeCleanWire()\n");
    TopoDS_Wire result;
    BRepBuilderAPI_MakeWire mkWire;
    ShapeFix_ShapeTolerance sTol;
    Handle(ShapeExtend_WireData) wireData = new ShapeExtend_WireData();

    for (const auto& e:edges) {
        wireData->Add(e);
    }

    Handle(ShapeFix_Wire) fixer = new ShapeFix_Wire;
    fixer->Load(wireData);
    fixer->SetPrecision(2.0 * EWTOLERANCE);
    fixer->SetMaxTolerance(tol);
    fixer->ClosedWireMode() = Standard_True;
    fixer->ModifyGeometryMode() = Standard_True;
    fixer->ModifyTopologyMode() = Standard_False;
    fixer->FixSelfIntersectingEdgeMode() = Standard_True;
    fixer->FixIntersectingEdgesMode() = Standard_True;
    fixer->FixIntersectingEdgesMode() = Standard_True;
    fixer->FixConnectedMode() = Standard_True;
    fixer->FixReorderMode() = Standard_True;
    fixer->Perform();

    return fixer->WireAPIMake();
}

std::vector<TopoDS_Vertex> EdgeWalker:: makeUniqueVList(std::vector<TopoDS_Edge> edges)
{
//    Base::Console().Message("TRACE - EW::makeUniqueVList() - edgesIn: %d\n", edges.size());
    std::vector<TopoDS_Vertex> uniqueVert;
    for(auto& e:edges) {
        Base::Vector3d v1 = DrawUtil::vertex2Vector(TopExp::FirstVertex(e));
        Base::Vector3d v2 = DrawUtil::vertex2Vector(TopExp::LastVertex(e));
        bool addv1 = true;
        bool addv2 = true;
        //check if we've already added this vertex
        for (const auto& v: uniqueVert) {
            Base::Vector3d v3d = DrawUtil::vertex2Vector(v);
            if (v3d.IsEqual(v1, EWTOLERANCE)) {
                addv1 = false;
            }
            if (v3d.IsEqual(v2, EWTOLERANCE)) {
                addv2 = false;
            }
        }
        if (addv1) {
            uniqueVert.push_back(TopExp::FirstVertex(e));
        }
        if (addv2) {
            uniqueVert.push_back(TopExp::LastVertex(e));
        }
    }
//    Base::Console().Message("EW::makeUniqueVList - verts out: %d\n", uniqueVert.size());
    return uniqueVert;
}

//!make WalkerEdges (unique Vertex index pairs) from edge list
std::vector<WalkerEdge> EdgeWalker::makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                                      std::vector<TopoDS_Vertex> verts)
{
//    Base::Console().Message("TRACE - EW::makeWalkerEdges() - edges: %d  verts: %d\n", edges.size(), verts.size());
    m_saveInEdges = edges;
    std::vector<WalkerEdge> walkerEdges;
    for (const auto& e:edges) {
        TopoDS_Vertex edgeVertex1 = TopExp::FirstVertex(e);
        TopoDS_Vertex edgeVertex2 = TopExp::LastVertex(e);
        std::size_t vertex1Index = findUniqueVert(edgeVertex1, verts);
        if (vertex1Index == SIZE_MAX) {
            continue;
        }
        std::size_t vertex2Index = findUniqueVert(edgeVertex2, verts);
        if (vertex2Index == SIZE_MAX) {
            continue;
        }

        WalkerEdge we;
        we.v1 = vertex1Index;
        we.v2 = vertex2Index;
        we.idx = 0;
        walkerEdges.push_back(we);
    }

    //Base::Console().Message("TRACE - EW::makeWalkerEdges - returns we: %d\n", walkerEdges.size());
    return walkerEdges;
}

size_t EdgeWalker::findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert)
{
//    Base::Console().Message("TRACE - EW::findUniqueVert()\n");
    std::size_t idx = 0;
    Base::Vector3d vx3d = DrawUtil::vertex2Vector(vx);
    for(auto& v : uniqueVert) {
        Base::Vector3d v3d = DrawUtil::vertex2Vector(v);
        if (vx3d.IsEqual(v3d, EWTOLERANCE)) {
            return idx;
        }
        idx++;
    }
    return SIZE_MAX;
}

std::vector<TopoDS_Wire> EdgeWalker::sortStrip(std::vector<TopoDS_Wire> fw, bool includeBiggest)
{
    std::vector<TopoDS_Wire> closedWires;                  //all the wires should be closed, but anomalies happen
    for (auto& w: fw) {
        if (!w.IsNull() && BRep_Tool::IsClosed(w)) {
            closedWires.push_back(w);
        }
    }
    std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(closedWires, false);           //biggest 1st
    if (sortedWires.empty()) {
        Base::Console().Message("EW::sortStrip - no sorted Wires!\n");
        return sortedWires;
    }

    if (!includeBiggest) {
        sortedWires.erase(sortedWires.begin());
    }

    return sortedWires;
}

// sort (closed) wires in order of enclosed area
std::vector<TopoDS_Wire> EdgeWalker::sortWiresBySize(std::vector<TopoDS_Wire>& w, bool ascend)
{
    //Base::Console().Message("TRACE - EW::sortWiresBySize()\n");
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), EdgeWalker::wireCompare);
    if (ascend) {
        std::reverse(wires.begin(), wires.end());
    }
    return wires;
}

//! return true if w1 enclosed area is bigger than w2 enclosed area
/*static*/bool EdgeWalker::wireCompare(const TopoDS_Wire& w1, const TopoDS_Wire& w2)
{
    double area1 = ShapeAnalysis::ContourArea(w1);
    double area2 = ShapeAnalysis::ContourArea(w2);
    return area1 > area2;
}

std::vector<embedItem> EdgeWalker::makeEmbedding(const std::vector<TopoDS_Edge> edges,
                                                 const std::vector<TopoDS_Vertex> uniqueVList)
{
//    Base::Console().Message("TRACE - EW::makeEmbedding(edges: %d, verts: %d)\n",
//                            edges.size(), uniqueVList.size());
    std::vector<embedItem> result;

    std::size_t iVert = 0;
    //make an embedItem for each vertex in uniqueVList
    //for each vertex v
    //  find all the edges that have v as first or last vertex
    for (auto& v: uniqueVList) {
        TopoDS_Vertex cv = v;               //v is const but we need non-const for vertexEqual
        std::size_t iEdge = 0;
        std::vector<incidenceItem> iiList;
        for (auto& e: edges) {
            double angle = 0;
            TopoDS_Vertex edgeVertex1 = TopExp::FirstVertex(e);
            TopoDS_Vertex edgeVertex2 = TopExp::LastVertex(e);
            if (DrawUtil::vertexEqual(cv, edgeVertex1)) {
                angle = DrawUtil::incidenceAngleAtVertex(e,v,EWTOLERANCE);
                incidenceItem ii(iEdge, angle, m_saveWalkerEdges[iEdge].ed);
                iiList.push_back(ii);
            } else if (DrawUtil::vertexEqual(cv, edgeVertex2)) {
                angle = DrawUtil::incidenceAngleAtVertex(e,v,EWTOLERANCE);
                incidenceItem ii(iEdge, angle, m_saveWalkerEdges[iEdge].ed);
                iiList.push_back(ii);
            }
            iEdge++;
       }
       //sort incidenceList by angle
       iiList = embedItem::sortIncidenceList(iiList,  false);
       embedItem embed(iVert, iiList);
       result.push_back(embed);
       iVert++;
    }
    return result;
}

//! get incidence row as edge indices for v'th vertex
std::vector<int> EdgeWalker::getEmbeddingRowIx(int v)
{
//    //Base::Console().Message("TRACE - EW::getEmbeddingRowIx(%d)\n", v);
    std::vector<int> result;
    embedItem ei = m_embedding[v];
    for (auto& ii: ei.incidenceList) {
        result.push_back(ii.iEdge);
    }
    return result;
}

//! get incidence row as edgeDescriptors for v'th vertex
std::vector<edge_t> EdgeWalker::getEmbeddingRow(int v)
{
//    //Base::Console().Message("TRACE - EW::getEmbeddingRow(%d)\n", v);
      std::vector<edge_t> result;
      embedItem ei = m_embedding[v];
      for (auto& ii: ei.incidenceList) {
          result.push_back(ii.eDesc);
      }
    return result;
}



//*******************************************
// WalkerEdge Methods
//*******************************************
bool WalkerEdge::isEqual(WalkerEdge w)
{
    if ((v1 == w.v1 && v2 == w.v2)  ||
        (v1 == w.v2 && v2 == w.v1) ) {
        return true;
    }
    return false;
}


/*static*/ bool WalkerEdge::weCompare(WalkerEdge i, WalkerEdge j)    //used for sorting
{
    return (i.idx < j.idx);
}

std::string WalkerEdge::dump()
{
    std::string result;
    std::stringstream builder;
    builder << "WalkerEdge - v1: " << v1  << " v2: " << v2 << " idx: " << idx << " ed: " << ed;
    return builder.str();
}

//*****************************************
// ewWire Methods
//*****************************************
bool ewWire::isEqual(ewWire w2)
{
    if (wedges.size() != w2.wedges.size()) {
        return false;
    }

    std::sort(wedges.begin(), wedges.end(), WalkerEdge::weCompare);
    std::sort(w2.wedges.begin(), w2.wedges.end(), WalkerEdge::weCompare);
    for (unsigned int i = 0; i < w2.wedges.size(); i ++) {
        if (wedges.at(i).idx != w2.wedges.at(i).idx) {
            return false;
        }
    }
    return true;
}

void ewWire::push_back(WalkerEdge w)
{
    wedges.push_back(w);
}

std::size_t ewWire::size(void)
{
    return wedges.size();
}

//***************************************
// ewWireList methods
//***************************************
//check wirelist for wires that use the same set of edges, but maybe in a different order.
ewWireList ewWireList::removeDuplicateWires()
{
    ewWireList result;
    if (wires.empty()) {
        return result;
    }
    result.push_back(*(wires.begin()));                //save the first ewWire
    std::vector<ewWire>::iterator iWire = (wires.begin()) + 1;    //starting with second
    for (; iWire != wires.end(); iWire++) {
        bool addToResult = true;
        for (auto& w:result.wires) {
            if ((*iWire).isEqual(w))  {             //already in result?
                addToResult = false;
                break;
            }
        }
        if (addToResult) {
            result.push_back((*iWire));
        }
    }
    return result;
}

void ewWireList::push_back(ewWire w)
{
    wires.push_back(w);
}

std::size_t ewWireList::size(void)
{
    return wires.size();
}

//*************************************
//* embedItem Methods
//*************************************

std::string embedItem::dump()
{
    std::string result;
    std::stringstream builder;
    builder << "embedItem - vertex: " << iVertex  << " incidenceList: ";
    for (auto& ii : incidenceList) {
        builder << " e:" << ii.iEdge << "/a:" << (ii.angle * (180.0/M_PI)) << "/ed:" << ii.eDesc;
    }
    return builder.str();
}

std::vector<incidenceItem> embedItem::sortIncidenceList (std::vector<incidenceItem> &list, bool ascend)
{
    //Base::Console().Message("TRACE - eI::sortIncidenceList()\n");
    std::vector< incidenceItem > tempList = list;
    std::sort(tempList.begin(), tempList.end(), incidenceItem::iiCompare);
    if (ascend) {
        std::reverse(tempList.begin(), tempList.end());
    }
    return tempList;
}

//*************************************
//* incidenceItem Methods
//*************************************

/*static*/  bool incidenceItem::iiCompare(const incidenceItem& i1, const incidenceItem& i2)
{
    return (i1.angle > i2.angle);
}

/*static*/bool incidenceItem::iiEqual(const incidenceItem& i1, const incidenceItem& i2)
{
    //TODO: this should compare edges also but eDesc comparison is by address
    if (i1.angle == i2.angle) {
    }
    return false;
}
