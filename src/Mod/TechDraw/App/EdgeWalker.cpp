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
#ifndef _PreComp_
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#endif
#include <sstream>
#include <cmath>

#include <Base/Console.h>
#include <Base/Exception.h>

#include "DrawUtil.h"
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
    we.ed = e;
    we.idx = get(edge_index,m_g,e);
    //Base::Console().Message("TRACE - EV::next_Edge - visiting (%d,%d) idx: %d\n",s,t,we.idx);
    wireEdges.push_back(we);
}

void edgeVisitor::begin_face()
{
    //Base::Console().Message("TRACE - EV::begin_face()\n");
    wireEdges.clear();
}

void edgeVisitor::end_face()
{
    //Base::Console().Message("TRACE - EV::end_face()\n");
    graphWires.push_back(wireEdges);
}

TechDraw::ewWireList edgeVisitor::getResult(void)
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
    //Base::Console().Message("TRACE -EW::loadEdges(we)\n");
    int idx = 0;
    for (auto& e: edges) {
        std::pair<edge_t,bool> p;
        p = add_edge(e.v1,e.v2,m_g);
        e.ed = p.first;
        e.idx = idx;
        idx++;
        m_saveWalkerEdges.push_back(e);
    }

    return true;
}

bool EdgeWalker::loadEdges(std::vector<TopoDS_Edge> edges)
{
    //Base::Console().Message("TRACE -EW::loadEdges(TopoDS)\n");
    if (edges.empty()) {
        throw Base::Exception("EdgeWalker has no edges to load\n");
    }

    std::vector<TopoDS_Vertex> verts = makeUniqueVList(edges);
    setSize(verts.size());

    std::vector<WalkerEdge>  we  = makeWalkerEdges(edges, verts);
    loadEdges(we);

    m_embedding = makeEmbedding(edges,verts);

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
    //Base::Console().Message("TRACE - EW::perform()\n");
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

    // Test for planarity
    typedef std::vector< graph_traits<TechDraw::graph>::edge_descriptor > vec_t;
    std::vector<vec_t> embedding(num_vertices(m_g));
    typedef std::vector< graph_traits<TechDraw::graph>::edge_descriptor > kura_edges_t;
    kura_edges_t kEdges;
    kura_edges_t::iterator ki, ki_end;
    graph_traits<TechDraw::graph>::edge_descriptor e1;

    // Get the index associated with edge
    graph_traits<TechDraw::graph>::edges_size_type
        get(boost::edge_index_t,
            const TechDraw::graph& m_g,
            graph_traits<TechDraw::graph>::edge_descriptor edge);

    bool isPlanar = boyer_myrvold_planarity_test(boyer_myrvold_params::graph = m_g,
                                 boyer_myrvold_params::embedding = &embedding[0],            // this is "an" embedding but not one for finding
                                 boyer_myrvold_params::kuratowski_subgraph =                 // closed regions in an edge pile.
                                       std::back_inserter(kEdges));
    if (!isPlanar) {
        //TODO: remove kura subgraph to make planar??
        Base::Console().Log("LOG - EW::perform - input is NOT planar\n");
        ki_end = kEdges.end();
        std::stringstream ss;
        ss << "EW::perform - obstructing edges: ";
        for(ki = kEdges.begin(); ki != ki_end; ++ki) {
            e1 = *ki;
            ss << boost::get(edge_index,m_g,e1) << ",";
        }
        ss << std::endl;
        Base::Console().Log("LOG - %s\n",ss.str().c_str());
        return false;
    }

    m_eV.setGraph(m_g);
    planar_face_traversal(m_g, &planar_embedding[0], m_eV);

    return true;
}

ewWireList EdgeWalker::getResult()
{
    //Base::Console().Message("TRACE - EW::getResult()\n");
    ewWireList result = m_eV.getResult();
    // result is a list of many wires each of which is a list of many WE
    return result;
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
    TopoDS_Wire w = makeCleanWire(topoEdges);             //make 1 clean wire from its edges
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
    TopoDS_Wire w = makeCleanWire(topoEdges);             //make 1 clean wire from its edges
    fw.push_back(w);
    }
    return fw;
}


//! make a clean wire with sorted, oriented, connected, etc edges
TopoDS_Wire EdgeWalker::makeCleanWire(std::vector<TopoDS_Edge> edges, double tol)
{
    //Base::Console().Message("TRACE - EW::makeCleanWire()\n");
    TopoDS_Wire result;
    BRepBuilderAPI_MakeWire mkWire;
    ShapeFix_ShapeTolerance sTol;
    Handle(ShapeExtend_WireData) wireData = new ShapeExtend_WireData();

    for (auto e:edges) {
        wireData->Add(e);
    }

    Handle(ShapeFix_Wire) fixer = new ShapeFix_Wire;
    fixer->Load(wireData);
    fixer->Perform();
    fixer->FixReorder();
    fixer->SetMaxTolerance(tol);
    fixer->ClosedWireMode() = Standard_True;
    fixer->FixConnected(Precision::Confusion());
    fixer->FixClosed(Precision::Confusion());

    for (int i = 1; i <= wireData->NbEdges(); i ++) {
        TopoDS_Edge edge = fixer->WireData()->Edge(i);
        sTol.SetTolerance(edge, tol, TopAbs_VERTEX);
        mkWire.Add(edge);
    }

    result = mkWire.Wire();
    return result;
}

std::vector<TopoDS_Vertex> EdgeWalker:: makeUniqueVList(std::vector<TopoDS_Edge> edges)
{
    //Base::Console().Message("TRACE - EW::makeUniqueVList()\n");
    std::vector<TopoDS_Vertex> uniqueVert;
    for(auto& e:edges) {
        TopoDS_Vertex v1 = TopExp::FirstVertex(e);
        TopoDS_Vertex v2 = TopExp::LastVertex(e);
        bool addv1 = true;
        bool addv2 = true;
        for (auto v:uniqueVert) {
            if (DrawUtil::isSamePoint(v,v1))
                addv1 = false;
            if (DrawUtil::isSamePoint(v,v2))
                addv2 = false;
        }
        if (addv1)
            uniqueVert.push_back(v1);
        if (addv2)
            uniqueVert.push_back(v2);
    }
    return uniqueVert;
}

//!make WalkerEdges (unique Vertex index pairs) from edge list
std::vector<WalkerEdge> EdgeWalker::makeWalkerEdges(std::vector<TopoDS_Edge> edges,
                                                      std::vector<TopoDS_Vertex> verts)
{
    //Base::Console().Message("TRACE - EW::makeWalkerEdges()\n");
    m_saveInEdges = edges;
    std::vector<WalkerEdge> walkerEdges;
    for (auto e:edges) {
        TopoDS_Vertex ev1 = TopExp::FirstVertex(e);
        TopoDS_Vertex ev2 = TopExp::LastVertex(e);
        int v1dx = findUniqueVert(ev1, verts);
        int v2dx = findUniqueVert(ev2, verts);
        WalkerEdge we;
        we.v1 = v1dx;
        we.v2 = v2dx;
        walkerEdges.push_back(we);
    }

    //Base::Console().Message("TRACE - EW::makeWalkerEdges - returns we: %d\n",walkerEdges.size());
    return walkerEdges;
}

int EdgeWalker::findUniqueVert(TopoDS_Vertex vx, std::vector<TopoDS_Vertex> &uniqueVert)
{
    //Base::Console().Message("TRACE - EW::findUniqueVert()\n");
    int idx = 0;
    int result = 0;
    for(auto& v:uniqueVert) {                    //we're always going to find vx, right?
        if (DrawUtil::isSamePoint(v,vx)) {
            result = idx;
            break;
        }
        idx++;
    }                                           //if idx >= uniqueVert.size() TARFU
    return result;
}

std::vector<TopoDS_Wire> EdgeWalker::sortStrip(std::vector<TopoDS_Wire> fw, bool includeBiggest)
{
    //Base::Console().Message("TRACE - EW::sortStrip()\n");
    std::vector<TopoDS_Wire> sortedWires = sortWiresBySize(fw,false);           //biggest 1st
    if (!sortedWires.size()) {
        Base::Console().Log("INFO - EW::sortStrip - no sorted Wires!\n");
        return sortedWires;                                     // might happen in the middle of changes?
    }

    //find the largest wire (OuterWire of graph) using bbox
    Bnd_Box bigBox;
    if (sortedWires.size() && !sortedWires.front().IsNull()) {
        BRepBndLib::Add(sortedWires.front(), bigBox);
        bigBox.SetGap(0.0);
    }
    std::vector<std::size_t> toBeChecked;
    std::vector<TopoDS_Wire>::iterator it = sortedWires.begin() + 1;
    for (; it != sortedWires.end(); it++) {
        if (!(*it).IsNull()) {
            Bnd_Box littleBox;
            BRepBndLib::Add((*it), littleBox);
            littleBox.SetGap(0.0);
            if (bigBox.SquareExtent() > littleBox.SquareExtent()) {
                break;
            } else {
                auto position = std::distance( sortedWires.begin(), it );    //get an index from iterator
                toBeChecked.push_back(position);
            }
        }
    }

    //unfortuneately, faces can have same bbox, but not be same size.  need to weed out biggest
    if (toBeChecked.size() == 0) {
        //nobody had as big a bbox as first element of sortedWires
        if (!includeBiggest) {
            sortedWires.erase(sortedWires.begin());
        }
    } else if (toBeChecked.size() > 0) {
        BRepBuilderAPI_MakeFace mkFace(sortedWires.front());
        const TopoDS_Face& face = mkFace.Face();
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        double bigArea = props.Mass();
        unsigned int bigIndex = 0;
        for (unsigned int idx = 0; idx < toBeChecked.size(); idx++) {
            int iCheck = toBeChecked.at(idx);
            BRepBuilderAPI_MakeFace mkFace2(sortedWires.at(iCheck));
            const TopoDS_Face& face2 = mkFace2.Face();
            BRepGProp::SurfaceProperties(face2, props);
            double area = props.Mass();
            if (area > bigArea) {
                bigArea = area;
                bigIndex = iCheck;
            }
        }
        if (bigIndex == 0) {                    //first wire is the biggest
            if (!includeBiggest) {
                sortedWires.erase(sortedWires.begin());
            }
        } else {                                  //first wire is not the biggest
            TopoDS_Wire bigWire = *(sortedWires.begin() + bigIndex);
            sortedWires.erase(sortedWires.begin() + bigIndex);
            if (includeBiggest) {
                sortedWires.insert(sortedWires.begin(),bigWire);               //doesn't happen often
            }
        }

    }
    return sortedWires;
}

//sort wires in order of bbox diagonal.
std::vector<TopoDS_Wire> EdgeWalker::sortWiresBySize(std::vector<TopoDS_Wire>& w, bool ascend)
{
    //Base::Console().Message("TRACE - EW::sortWiresBySize()\n");
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), EdgeWalker::wireCompare);
    if (ascend) {
        std::reverse(wires.begin(),wires.end());
    }
    return wires;
}

//! return true if w1 bbox is bigger than w2 bbox
//NOTE: this won't necessarily sort the OuterWire correctly (ex smaller wire, same bbox)
/*static*/bool EdgeWalker::wireCompare(const TopoDS_Wire& w1, const TopoDS_Wire& w2)
{
    Bnd_Box box1, box2;
    if (!w1.IsNull()) {
        BRepBndLib::Add(w1, box1);
        box1.SetGap(0.0);
    }

    if (!w2.IsNull()) {
        BRepBndLib::Add(w2, box2);
        box2.SetGap(0.0);
    }

    return box1.SquareExtent() > box2.SquareExtent();
}

std::vector<embedItem> EdgeWalker::makeEmbedding(const std::vector<TopoDS_Edge> edges,
                                                 const std::vector<TopoDS_Vertex> uniqueVList)
{
//    Base::Console().Message("TRACE - EW::makeEmbedding(edges: %d, verts: %d)\n",
//                            edges.size(),uniqueVList.size());
    std::vector<embedItem> result;

    int iv = 0;
    for (auto& v: uniqueVList) {
        int ie = 0;
        std::vector<incidenceItem> iiList;
        for (auto& e: edges) {
            double angle = 0;
            if (DrawUtil::isFirstVert(e,v)) {
                angle = DrawUtil::angleWithX(e,v);
                incidenceItem ii(ie, angle, m_saveWalkerEdges[ie].ed);
                iiList.push_back(ii);
            } else if (DrawUtil::isLastVert(e,v)) {
                angle = DrawUtil::angleWithX(e,v);
                incidenceItem ii(ie, angle, m_saveWalkerEdges[ie].ed);
                iiList.push_back(ii);
            } else {
                //Base::Console().Message("TRACE - EW::makeEmbedding - neither first nor last\n");
            }
            ie++;
       }
       //sort incidenceList by angle
       iiList = embedItem::sortIncidenceList(iiList,  false);
       embedItem embed(iv, iiList);
       result.push_back(embed);
       iv++;
    }
    return result;
}

//! get incidence row as edge indices for v'th vertex
std::vector<int> EdgeWalker::getEmbeddingRowIx(int v)
{
//    //Base::Console().Message("TRACE - EW::getEmbeddingRowIx(%d)\n",v);
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
//    //Base::Console().Message("TRACE - EW::getEmbeddingRow(%d)\n",v);
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
    bool result = false;
    if ((( v1 == w.v1) && (v2 == w.v2))  ||
        (( v1 == w.v2) && (v2 == w.v1)) ) {
        result = true;
    }
    return result;
}


/*static*/ bool WalkerEdge::weCompare(WalkerEdge i, WalkerEdge j)    //used for sorting
{
    return (i.idx < j.idx);
}


//*****************************************
// ewWire Methods
//*****************************************
bool ewWire::isEqual(ewWire w2)
{
    bool result = true;
    if (wedges.size() != w2.wedges.size()) {
        result = false;
    } else {
        std::sort(wedges.begin(),wedges.end(),WalkerEdge::weCompare);
        std::sort(w2.wedges.begin(),w2.wedges.end(),WalkerEdge::weCompare);
        for (unsigned int i = 0; i < w2.wedges.size(); i ++) {
            if (wedges.at(i).idx != w2.wedges.at(i).idx) {
                result = false;
                break;
            }
        }
    }
    return result;
}

void ewWire::push_back(WalkerEdge w)
{
    wedges.push_back(w);
}

int ewWire::size(void)
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

int ewWireList::size(void)
{
    return wires.size();
}

//*************************************
//* embedItem Methods
//*************************************

std::string embedItem::dump(void)
{
    std::string result;
    std::stringstream builder;
    builder << "embedItem - vertex: " << iVertex  << " incidenceList: ";
    for (auto& ii : incidenceList) {
        builder << "e:" << ii.iEdge << "/a:" << (ii.angle * (180.0/M_PI)) << "/ed: " << ii.eDesc;
    }
    result = builder.str();
    return result;
}

/*static*/  bool embedItem::iiCompare(incidenceItem i1, incidenceItem i2)
{
    return (i1.angle > i2.angle);
}

std::vector<incidenceItem> embedItem::sortIncidenceList (std::vector<incidenceItem> &list, bool ascend)
{
    //Base::Console().Message("TRACE - eI::sortIncidenceList()\n");
    std::vector< incidenceItem > tempList = list;
    std::sort(tempList.begin(), tempList.end(), embedItem::iiCompare);
    if (ascend) {
        std::reverse(tempList.begin(),tempList.end());
    }
    return tempList;
}
