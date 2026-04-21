// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#include <bitset>
#include <memory>

#include <boost_graph_adjacency_list.hpp>
#include <boost_graph_reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <fastsignals/signal.h>

#include "DAGRectItem.h"


namespace App
{
class DocumentObject;
}

namespace Gui
{
class ViewProviderDocumentObject;

namespace DAG
{
enum class VisibilityState
{
    None = 0,  //<! not determined.
    On,        //<! shown
    Off        //<! hidden
};

enum class FeatureState
{
    None = 0,  //<! not determined.
    Pass,      //<! feature updated ok.
    Fail,      //<! feature failed to update.
    Pending    //<! feature is pending an update.
};

// limit of column width? boost::dynamic_bitset?
// did a trial run with this set at 4096, not much difference.
// going to leave a big number by default and see how it goes.
using ColumnMask = std::bitset<1024>;

/*! @brief Graph vertex information
 *
 * My data stored for each vertex;
 */
struct VertexProperty
{
    VertexProperty();
    std::shared_ptr<RectItem> rectangle;               //!< background
    std::shared_ptr<QGraphicsEllipseItem> point;       //!< point
    std::shared_ptr<QGraphicsPixmapItem> visibleIcon;  //!< visible Icon
    std::shared_ptr<QGraphicsPixmapItem> stateIcon;    //!< visible Icon
    std::shared_ptr<QGraphicsPixmapItem> icon;         //!< icon
    std::shared_ptr<QGraphicsTextItem> text;           //!< text
    fastsignals::connection connChangeIcon;
    int row = 0;            //!< row for this entry.
    ColumnMask column = 0;  //!< column number containing the point.
    int topoSortIndex = 0;
    VisibilityState lastVisibleState = VisibilityState::None;  //!< visibility test.
    FeatureState lastFeatureState = FeatureState::None;        //!< feature state test.
};
/*! @brief boost data for each vertex.
 *
 * needed to create an internal index for vertex. needed for listS.
 * color is needed by some algorithms */
using vertex_prop = boost::property<
    boost::vertex_index_t,
    std::size_t,
    boost::property<boost::vertex_color_t, boost::default_color_type, VertexProperty>>;

/*! @brief Graph edge information
 *
 * My data stored for each edge;
 */
struct EdgeProperty
{
    //! Feature relation meta data. Not used right now.
    enum class BranchTag
    {
        None = 0,  //!< not defined.
        Create,    //!< create a new branch.
        Continue,  //!< continue a branch.
        Terminate  //!< terminate a branch.
    };
    EdgeProperty();
    BranchTag relation = BranchTag::None;
    std::shared_ptr<QGraphicsPathItem> connector;  //!< line representing link between nodes.
};
/*! @brief needed to create an internal index for graph edges. needed for setS.*/
using edge_prop = boost::property<boost::edge_index_t, std::size_t, EdgeProperty>;

using Graph
    = boost::adjacency_list<boost::setS, boost::listS, boost::bidirectionalS, vertex_prop, edge_prop>;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using Edge = boost::graph_traits<Graph>::edge_descriptor;
using VertexIterator = boost::graph_traits<Graph>::vertex_iterator;
using EdgeIterator = boost::graph_traits<Graph>::edge_iterator;
using InEdgeIterator = boost::graph_traits<Graph>::in_edge_iterator;
using OutEdgeIterator = boost::graph_traits<Graph>::out_edge_iterator;
using VertexAdjacencyIterator = boost::graph_traits<Graph>::adjacency_iterator;
using GraphReversed = boost::reverse_graph<Graph, Graph&>;
using Path = std::vector<Vertex>;  //!< a path or any array of vertices

template<class GraphEW>
class Edge_writer
{
public:
    explicit Edge_writer(const GraphEW& graphEWIn)
        : graphEW(graphEWIn)
    {}
    template<class EdgeW>
    void operator()(std::ostream& out, const EdgeW& /*edgeW*/) const
    {
        out << "[label=\"";
        out << "edge";
        out << "\"]";
    }

private:
    const GraphEW& graphEW;
};

template<class GraphVW>
class Vertex_writer
{
public:
    explicit Vertex_writer(const GraphVW& graphVWIn)
        : graphVW(graphVWIn)
    {}
    template<class VertexW>
    void operator()(std::ostream& out, const VertexW& vertexW) const
    {
        out << "[label=\"";
        out << graphVW[vertexW].text->toPlainText().toLatin1().data();
        out << "\"]";
    }

private:
    const GraphVW& graphVW;
};

template<class GraphIn>
void outputGraphviz(const GraphIn& graphIn, const std::string& filePath)
{
    std::ofstream file(filePath.c_str());
    boost::write_graphviz(file, graphIn, Vertex_writer<GraphIn>(graphIn), Edge_writer<GraphIn>(graphIn));
}

//! get all the leaves of the templated graph. Not used right now.
template<class GraphIn>
class RakeLeaves
{
    using GraphInVertex = boost::graph_traits<Graph>::vertex_descriptor;
    using GraphInVertices = std::vector<GraphInVertex>;

public:
    explicit RakeLeaves(const GraphIn& graphIn)
        : graph(graphIn)
    {}
    GraphInVertices operator()() const
    {
        GraphInVertices out;
        BGL_FORALL_VERTICES_T(currentVertex, graph, GraphIn)
        {
            if (boost::out_degree(currentVertex, graph) == 0) {
                out.push_back(currentVertex);
            }
        }
        return out;
    }

private:
    const GraphIn& graph;
};

//! get all the roots of the templated graph. Not used right now.
template<class GraphIn>
class DigRoots
{
    using GraphInVertex = boost::graph_traits<Graph>::vertex_descriptor;
    using GraphInVertices = std::vector<GraphInVertex>;

public:
    explicit DigRoots(const GraphIn& graphIn)
        : graph(graphIn)
    {}
    GraphInVertices operator()() const
    {
        GraphInVertices out;
        BGL_FORALL_VERTICES_T(currentVertex, graph, GraphIn)
        {
            if (boost::in_degree(currentVertex, graph) == 0) {
                out.push_back(currentVertex);
            }
        }
        return out;
    }

private:
    const GraphIn& graph;
};

/*! @brief Get connected components.
 */
class ConnectionVisitor: public boost::default_bfs_visitor
{
public:
    explicit ConnectionVisitor(std::vector<Vertex>& verticesIn)
        : vertices(verticesIn)
    {}

    template<typename TVertex, typename TGraph>
    void discover_vertex(TVertex vertex, TGraph& graph)
    {
        Q_UNUSED(graph);
        vertices.push_back(vertex);
    }

private:
    std::vector<Vertex>& vertices;
};

/*! Multi_index record. */
struct GraphLinkRecord
{
    const App::DocumentObject* DObject;                //!< document object
    const Gui::ViewProviderDocumentObject* VPDObject;  //!< view provider
    const RectItem* rectItem;                          //!< qgraphics item.
    std::string uniqueName;                            //!< name for document object.
    Vertex vertex;                                     //!< vertex in graph.

    //@{
    //! used as tags.
    struct ByDObject
    {
    };
    struct ByVPDObject
    {
    };
    struct ByRectItem
    {
    };
    struct ByUniqueName
    {
    };
    struct ByVertex
    {
    };
    //@}
};

namespace BMI = boost::multi_index;
using GraphLinkContainer = boost::multi_index_container<
    GraphLinkRecord,
    BMI::indexed_by<
        BMI::ordered_unique<
            BMI::tag<GraphLinkRecord::ByDObject>,
            BMI::member<GraphLinkRecord, const App::DocumentObject*, &GraphLinkRecord::DObject>>,
        BMI::ordered_unique<
            BMI::tag<GraphLinkRecord::ByVPDObject>,
            BMI::member<GraphLinkRecord, const Gui::ViewProviderDocumentObject*, &GraphLinkRecord::VPDObject>>,
        BMI::ordered_unique<
            BMI::tag<GraphLinkRecord::ByRectItem>,
            BMI::member<GraphLinkRecord, const RectItem*, &GraphLinkRecord::rectItem>>,
        BMI::ordered_unique<
            BMI::tag<GraphLinkRecord::ByUniqueName>,
            BMI::member<GraphLinkRecord, std::string, &GraphLinkRecord::uniqueName>>,
        BMI::ordered_unique<
            BMI::tag<GraphLinkRecord::ByVertex>,
            BMI::member<GraphLinkRecord, Vertex, &GraphLinkRecord::vertex>>>>;

bool hasRecord(const App::DocumentObject* dObjectIn, const GraphLinkContainer& containerIn);
bool hasRecord(const ViewProviderDocumentObject* VPDObjectIn, const GraphLinkContainer& containerIn);
const GraphLinkRecord& findRecord(Vertex vertexIn, const GraphLinkContainer& containerIn);
const GraphLinkRecord& findRecord(
    const App::DocumentObject* dObjectIn,
    const GraphLinkContainer& containerIn
);
const GraphLinkRecord& findRecord(
    const Gui::ViewProviderDocumentObject* VPDObjectIn,
    const GraphLinkContainer& containerIn
);
const GraphLinkRecord& findRecord(const RectItem* rectIn, const GraphLinkContainer& containerIn);
const GraphLinkRecord& findRecord(const std::string& stringIn, const GraphLinkContainer& containerIn);
void eraseRecord(const Gui::ViewProviderDocumentObject* VPDObjectIn, GraphLinkContainer& containerIn);
}  // namespace DAG
}  // namespace Gui
