/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PreCompiled.h"

#include <boost/graph/graphviz.hpp>
#include <random>

#include "Application.h"
#include "Document.h"
#include "private/DocumentP.h"
#include "DocumentObject.h"
#include "ExpressionParser.h"
#include "GeoFeatureGroupExtension.h"
#include "Origin.h"
#include "OriginGroupExtension.h"
#include "ObjectIdentifier.h"

using namespace App;
using namespace boost;

void Document::writeDependencyGraphViz(std::ostream &out)
{
    //  // caching vertex to DocObject
    //std::map<Vertex,DocumentObject*> VertexMap;
    //for(std::map<DocumentObject*,Vertex>::const_iterator It1= _DepConMap.begin();It1 != _DepConMap.end(); ++It1)
    //  VertexMap[It1->second] = It1->first;

    out << "digraph G {" << std::endl;
    out << "\tordering=out;" << std::endl;
    out << "\tnode [shape = box];" << std::endl;

    for (const auto &It : d->objectMap) {
        out << "\t" << It.first << ";" << std::endl;
        std::vector<DocumentObject*> OutList = It.second->getOutList();
        for (const auto &It2 : OutList) {
            if (It2) {
                out << "\t" << It.first << "->" << It2->getNameInDocument() << ";" << std::endl;
            }
        }
    }

    /*
    graph_traits<DependencyList>::edge_iterator ei, ei_end;
    for (tie(ei,ei_end) = edges(_DepList); ei != ei_end; ++ei)
      out << "\t"
          << VertexMap[source(*ei, _DepList)]->getNameInDocument()
          << " -> "
          << VertexMap[target(*ei, _DepList)]->getNameInDocument()
          << ";" << endl;
    */
    out << "}" << std::endl;
}

void Document::exportGraphviz(std::ostream& out) const
{
    /* Type defs for a graph with graphviz attributes */
    using GraphvizAttributes = std::map<std::string, std::string>;
    using Graph = boost::subgraph< adjacency_list<vecS, vecS, directedS,
                  property<vertex_attribute_t, GraphvizAttributes>,
                  property<edge_index_t, int, property<edge_attribute_t, GraphvizAttributes> >,
                  property<graph_name_t, std::string,
                  property<graph_graph_attribute_t,  GraphvizAttributes,
                  property<graph_vertex_attribute_t, GraphvizAttributes,
                  property<graph_edge_attribute_t,   GraphvizAttributes>
                  > > > > >;

    /**
     * @brief The GraphCreator class
     *
     * This class creates the dependency graph for a document.
     *
     */

    class GraphCreator {
    public:

        explicit GraphCreator(struct DocumentP* _d) : d(_d), seed(std::random_device()()), distribution(0,255) {
            build();
        }

        const Graph & getGraph() const { return DepList; }

    private:

        void build() {
            // Set attribute(s) for main graph
            get_property(DepList, graph_graph_attribute)["compound"] = "true";

            addSubgraphs();
            buildAdjacencyList();
            addEdges();
            markCycles();
            markOutOfScopeLinks();
        }

        /**
         * @brief getId returns a canonical string for a DocumentObject.
         * @param docObj Document object to get an ID from
         * @return A string
         */

        std::string getId(const DocumentObject * docObj) {
            return std::string((docObj)->getDocument()->getName()) + "#" + docObj->getNameInDocument();
        }

        /**
         * @brief getId returns a canonical string for an ObjectIdentifier;
         * @param path
         * @return A string
         */

        std::string getId(const ObjectIdentifier & path) {
            DocumentObject * docObj = path.getDocumentObject();
            if (!docObj)
                return {};

            return std::string((docObj)->getDocument()->getName()) + "#" + docObj->getNameInDocument() + "." + path.getPropertyName() + path.getSubPathStr();
        }

        std::string getClusterName(const DocumentObject * docObj) const {
            return std::string("cluster") + docObj->getNameInDocument();
        }

        void setGraphLabel(Graph& g, const DocumentObject* obj) const {
            std::string name(obj->getNameInDocument());
            std::string label(obj->Label.getValue());
            if (name == label)
                get_property(g, graph_graph_attribute)["label"] = name;
            else
                get_property(g, graph_graph_attribute)["label"] = name + "&#92;n(" + label + ")";
        }

        /**
         * @brief setGraphAttributes Set graph attributes on a subgraph for a DocumentObject node.
         * @param obj DocumentObject
         */

        void setGraphAttributes(const DocumentObject * obj) {
            assert(GraphList.find(obj) != GraphList.end());
            get_property(*GraphList[obj], graph_name) = getClusterName(obj);

            get_property(*GraphList[obj], graph_graph_attribute)["bgcolor"] = "#e0e0e0";

            get_property(*GraphList[obj], graph_graph_attribute)["style"] = "rounded,filled";
            setGraphLabel(*GraphList[obj], obj);
        }

        /**
         * @brief setPropertyVertexAttributes Set vertex attributes for a Property node in a graph.
         * @param g Graph
         * @param vertex Property node
         * @param name Name of node
         */

        void setPropertyVertexAttributes(Graph & g, Vertex vertex, const std::string & name) {
            get(vertex_attribute, g)[vertex]["label"] = name;
            get(vertex_attribute, g)[vertex]["shape"] = "box";
            get(vertex_attribute, g)[vertex]["style"] = "dashed";
            get(vertex_attribute, g)[vertex]["fontsize"] = "8pt";
        }

        /**
         * @brief addExpressionSubgraphIfNeeded Add a subgraph to the main graph if it is needed, i.e. there are defined at least one
         * expression in the document object, or other objects are referencing properties in it.
         * @param obj DocumentObject to assess.
         * @param CSSubgraphs Boolean if the GeoFeatureGroups are created as subgraphs
         */

        void addExpressionSubgraphIfNeeded(DocumentObject * obj, bool CSsubgraphs) {

            auto expressions = obj->ExpressionEngine.getExpressions();

            if (!expressions.empty()) {

                Graph* graph = nullptr;
                graph = &DepList;
                if (CSsubgraphs) {
                    auto group = GeoFeatureGroupExtension::getGroupOfObject(obj);
                    if (group) {
                        auto it = GraphList.find(group);
                        if (it != GraphList.end())
                            graph = it->second;
                    }
                }

                // If documentObject has an expression, create a subgraph for it
                if (graph && !GraphList[obj]) {
                    GraphList[obj] = &graph->create_subgraph();
                    setGraphAttributes(obj);
                }

                // Create subgraphs for all document objects that it depends on; it will depend on some property there
                for (const auto &expr : expressions) {
                    std::map<ObjectIdentifier, bool> deps;

                    expr.second->getIdentifiers(deps);

                    for (const auto &dep : deps) {
                        if (dep.second)
                            continue;
                        DocumentObject * o = dep.first.getDocumentObject();

                        // Doesn't exist already?
                        if (o && !GraphList[o]) {

                            if (CSsubgraphs) {
                                auto group = GeoFeatureGroupExtension::getGroupOfObject(o);
                                auto graph2 = group ? GraphList[group] : &DepList;
                                if (graph2) {
                                    GraphList[o] = &graph2->create_subgraph();
                                    setGraphAttributes(o);
                                }
                            }
                            else if (graph) {
                                GraphList[o] = &graph->create_subgraph();
                                setGraphAttributes(o);
                            }
                        }
                    }
                }
            }
        }

        /**
         * @brief add Add @docObj to the graph, including all expressions (and dependencies) it includes.
         * @param docObj The document object to add.
         * @param name Name of node.
         */

        void add(DocumentObject *docObj, const std::string &name, const std::string &label, bool CSSubgraphs)
        {

            //don't add objects twice
            if (std::find(objects.begin(), objects.end(), docObj) != objects.end())
                return;

            //find the correct graph to add the vertex to. Check first expression graphs, afterwards
            //the parent CS and origin graphs
            Graph *sgraph = GraphList[docObj];
            if (CSSubgraphs) {
                if (!sgraph) {
                    auto group = GeoFeatureGroupExtension::getGroupOfObject(docObj);
                    if (group) {
                        if (docObj->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                            sgraph = GraphList[group->getExtensionByType<OriginGroupExtension>()->Origin.getValue()];
                        else
                            sgraph = GraphList[group];
                    }
                }
                if (!sgraph) {
                    if (docObj->isDerivedFrom(OriginFeature::getClassTypeId()))
                        sgraph = GraphList[static_cast<OriginFeature *>(docObj)->getOrigin()];
                }
            }
            if (!sgraph)
                sgraph = &DepList;

            // Keep a list of all added document objects.
            objects.insert(docObj);

            // Add vertex to graph. Track global and local index
            LocalVertexList[getId(docObj)] = add_vertex(*sgraph);
            GlobalVertexList[getId(docObj)] = vertex_no++;

            // If node is in main graph, style it with rounded corners. If not, make it invisible.
            if (!GraphList[docObj]) {
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["style"] = "filled";
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["shape"] = "Mrecord";
                // Set node label
                if (name == label)
                    get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["label"] = name;
                else
                    get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["label"] = name + "&#92;n(" + label + ")";
            }
            else {
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["style"] = "invis";
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["fixedsize"] = "true";
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["width"] = "0";
                get(vertex_attribute, *sgraph)[LocalVertexList[getId(docObj)]]["height"] = "0";
            }

            // Add expressions and its dependencies
            auto expressions{docObj->ExpressionEngine.getExpressions()};
            for (const auto &expr : expressions) {
                auto found = std::as_const(GlobalVertexList).find(getId(expr.first));
                if (found == GlobalVertexList.end()) {
                    int vid = LocalVertexList[getId(expr.first)] = add_vertex(*sgraph);
                    GlobalVertexList[getId(expr.first)] = vertex_no++;
                    setPropertyVertexAttributes(*sgraph, vid, expr.first.toString());
                }
            }

            // Add all dependencies
            for (const auto &expression : expressions) {
                // Get dependencies
                std::map<ObjectIdentifier, bool> deps;
                expression.second->getIdentifiers(deps);

                // Create subgraphs for all documentobjects that it depends on; it will depend on some property there
                for (const auto &dep : deps) {
                    if (dep.second) {
                        continue;
                    }
                    DocumentObject *depObjDoc = dep.first.getDocumentObject();
                    auto found = GlobalVertexList.find(getId(dep.first));

                    if (found == GlobalVertexList.end()) {
                        Graph *depSgraph = GraphList[depObjDoc] ? GraphList[depObjDoc] : &DepList;

                        LocalVertexList[getId(dep.first)] = add_vertex(*depSgraph);
                        GlobalVertexList[getId(dep.first)] = vertex_no++;
                        setPropertyVertexAttributes(*depSgraph, LocalVertexList[getId(dep.first)], dep.first.getPropertyName() + dep.first.getSubPathStr());
                    }
                }
            }
        }

        void recursiveCSSubgraphs(DocumentObject* cs, DocumentObject* parent) {

            auto graph = parent ? GraphList[parent] : &DepList;
            // check if the value for the key 'parent' is null
            if (!graph)
                return;
            auto& sub = graph->create_subgraph();
            GraphList[cs] = &sub;
            get_property(sub, graph_name) = getClusterName(cs);

            //build random color string
            std::stringstream stream;
            stream << "#" << std::setfill('0') << std::setw(2)<< std::hex << distribution(seed)
                   << std::setfill('0') << std::setw(2)<< std::hex << distribution(seed)
                   << std::setfill('0') << std::setw(2)<< std::hex << distribution(seed) << 80;
            std::string result(stream.str());

            get_property(sub, graph_graph_attribute)["bgcolor"] = result;
            get_property(sub, graph_graph_attribute)["style"] = "rounded,filled";
            setGraphLabel(sub, cs);

            for(auto obj : cs->getOutList()) {
                if (obj->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId())) {
                    // in case of dependencies loops check if obj is already part of the
                    // map to avoid infinite recursions
                    auto it = GraphList.find(obj);
                    if (it == GraphList.end())
                        recursiveCSSubgraphs(obj, cs);
                }
            }

            //setup the origin if available
            if(cs->hasExtension(App::OriginGroupExtension::getExtensionClassTypeId())) {
                auto origin = cs->getExtensionByType<OriginGroupExtension>()->Origin.getValue();
                if (!origin) {
                    std::cerr << "Origin feature not found" << std::endl;
                    return;
                }
                auto& osub = sub.create_subgraph();
                GraphList[origin] = &osub;
                get_property(osub, graph_name) = getClusterName(origin);
                get_property(osub, graph_graph_attribute)["bgcolor"] = "none";
                setGraphLabel(osub, origin);
            }
        }

        void addSubgraphs() {

            ParameterGrp::handle depGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DependencyGraph");
            bool CSSubgraphs = depGrp->GetBool("GeoFeatureSubgraphs", true);

            if(CSSubgraphs) {
                //first build up the coordinate system subgraphs
                for (auto objectIt : d->objectArray) {
                    // do not require an empty inlist (#0003465: Groups breaking dependency graph)
                    // App::Origin now has the GeoFeatureGroupExtension but it should not move its
                    // group symbol outside its parent
                    if (!objectIt->isDerivedFrom(Origin::getClassTypeId()) &&
                         objectIt->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
                        recursiveCSSubgraphs(objectIt, nullptr);
                }
            }

            // Internal document objects
            for (const auto & It : d->objectMap)
                addExpressionSubgraphIfNeeded(It.second, CSSubgraphs);

            // Add external document objects
            for (const auto & it : d->objectMap) {
                std::vector<DocumentObject*> OutList = it.second->getOutList();
                for (auto obj : OutList) {
                    if (obj) {
                        std::map<std::string,Vertex>::const_iterator item = GlobalVertexList.find(getId(obj));

                        if (item == GlobalVertexList.end())
                            addExpressionSubgraphIfNeeded(obj, CSSubgraphs);
                    }
                }
            }

        }

        // Filling up the adjacency List
        void buildAdjacencyList() {

            ParameterGrp::handle depGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DependencyGraph");
            bool CSSubgraphs = depGrp->GetBool("GeoFeatureSubgraphs", true);

            // Add internal document objects
            for (const auto & It : d->objectMap)
                add(It.second, It.second->getNameInDocument(), It.second->Label.getValue(), CSSubgraphs);

            // Add external document objects
            for (const auto & It : d->objectMap) {
                std::vector<DocumentObject*> OutList = It.second->getOutList();
                for (auto obj : OutList) {
                    if (obj) {
                        std::map<std::string,Vertex>::const_iterator item = GlobalVertexList.find(getId(obj));

                        if (item == GlobalVertexList.end())
                            add(obj,
                                std::string(obj->getDocument()->getName()) + "#" + obj->getNameInDocument(),
                                std::string(obj->getDocument()->getName()) + "#" + obj->Label.getValue(),
                                CSSubgraphs);
                    }
                }
            }
        }

        void addEdges() {
            // Get edge properties for main graph
            const boost::property_map<Graph, boost::edge_attribute_t>::type& edgeAttrMap = boost::get(boost::edge_attribute, DepList);

            // Track edges between document objects connected by expression dependencies
            std::set<std::pair<const DocumentObject*, const DocumentObject*> > existingEdges;

            // Add edges between properties
            for (const auto &docObj : objects) {

                // Add expressions and its dependencies
                auto expressions = docObj->ExpressionEngine.getExpressions();
                for (const auto &expr : expressions) {
                    std::map<ObjectIdentifier, bool> deps;
                    expr.second->getIdentifiers(deps);

                    // Create subgraphs for all documentobjects that it depends on; it will depend on some property there
                    for (const auto &dep : deps) {
                        if (dep.second)
                            continue;
                        DocumentObject * depObjDoc = dep.first.getDocumentObject();
                        Edge edge;
                        bool inserted;

                        tie(edge, inserted) = add_edge(GlobalVertexList[getId(expr.first)], GlobalVertexList[getId(dep.first)], DepList);

                        // Add this edge to the set of all expression generated edges
                        existingEdges.insert(std::make_pair(docObj, depObjDoc));

                        // Edges between properties should be a bit smaller, and dashed
                        edgeAttrMap[edge]["arrowsize"] = "0.5";
                        edgeAttrMap[edge]["style"] = "dashed";
                    }
                }
            }

            ParameterGrp::handle depGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DependencyGraph");
            bool omitGeoFeatureGroups = depGrp->GetBool("GeoFeatureSubgraphs", true);

            // Add edges between document objects
            for (const auto & It : d->objectMap) {

                if(omitGeoFeatureGroups) {
                    //coordinate systems are represented by subgraphs
                    if(It.second->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
                        continue;

                    //as well as origins
                    if(It.second->isDerivedFrom(Origin::getClassTypeId()))
                        continue;
                }

                std::map<DocumentObject*, int> dups;
                std::vector<DocumentObject*> OutList = It.second->getOutList();
                const DocumentObject * docObj = It.second;

                for (auto obj : OutList) {
                    if (obj) {

                        // Count duplicate edges
                        bool inserted = edge(GlobalVertexList[getId(docObj)], GlobalVertexList[getId(obj)], DepList).second;
                        if (inserted) {
                            dups[obj]++;
                            continue;
                        }

                        // Skip edge if an expression edge already exists
                        if (existingEdges.find(std::make_pair(docObj, obj)) != existingEdges.end())
                            continue;

                        // Add edge

                        Edge edge;

                        tie(edge, inserted) = add_edge(GlobalVertexList[getId(docObj)], GlobalVertexList[getId(obj)], DepList);

                        // Set properties to make arrows go between subgraphs if needed
                        if (GraphList[docObj])
                            edgeAttrMap[edge]["ltail"] = getClusterName(docObj);
                        if (GraphList[obj])
                            edgeAttrMap[edge]["lhead"] = getClusterName(obj);
                    }
                }

                // Set labels for duplicate edges
                for (const auto & dup : dups) {
                    Edge e(edge(GlobalVertexList[getId(It.second)], GlobalVertexList[getId(dup.first)], DepList).first);
                    std::stringstream s;
                    s << " " << (dup.second + 1) << "x";
                    edgeAttrMap[e]["label"] = s.str();
                }
            }
        }

        using EdgeMap = std::unordered_multimap<Vertex, Edge>;

        void removeEdges(EdgeMap & in_edges,
                         EdgeMap & out_edges,
                         std::pair<EdgeMap::iterator, EdgeMap::iterator > i_pair,
                         std::function<Vertex (const Edge&)> select_vertex) {
            auto i = i_pair.first;

            while (i != i_pair.second) {
                // Remove from in edges in other nodes
                auto in_i_pair = in_edges.equal_range(select_vertex(i->second));
                auto in_i = in_i_pair.first;

                while (in_i != in_i_pair.second) {
                    if (in_i->second == i->second)
                        in_i = in_edges.erase(in_i);
                    else
                        ++in_i;
                }

                // Remove node from out_edges
                i = out_edges.erase(i);
            }
        }

#if defined(__clang__)
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

        void markCycles() {
            bool changed = true;
            std::unordered_set<Vertex> in_use;
            EdgeMap in_edges;
            EdgeMap out_edges;

            // Add all vertices to the in_use set
            graph_traits<Graph>::vertex_iterator vi, vi_end;
            tie(vi, vi_end) = vertices(DepList);
            for (; vi != vi_end; ++vi)
                in_use.insert(*vi);

            // Add all edges to the in_edges and out_edges multimaps
            graph_traits<Graph>::edge_iterator ei, ei_end;
            tie(ei, ei_end) = edges(DepList);
            for (; ei != ei_end; ++ei) {
                in_edges.insert(std::make_pair(target(*ei, DepList), *ei));
                out_edges.insert(std::make_pair(source(*ei, DepList), *ei));
            }

            // Go through dependency graph and remove nodes with either no input or output
            // A normal DAG without any cycles will get all its edges removed.
            // If one or more cycles exist in the graph, there will remain nodes with
            // both in and out edges.

            while (changed) {
                auto uvi = in_use.begin();
                auto uvi_end = in_use.end();

                // Flag that no changes has occurred so far. If the loop goes through
                // without this flag being set to true, we are done.
                changed = false;

                while (uvi != uvi_end) {
                    auto i_in_deg_pair = in_edges.equal_range(*uvi);
                    auto i_out_deg_pair = out_edges.equal_range(*uvi);

                    if (i_in_deg_pair.first == in_edges.end() && i_out_deg_pair.first == out_edges.end()) {
                        uvi = in_use.erase(uvi);
                        continue;
                    }

                    // Remove out edges of nodes that don't have a single edge in
                    if (i_in_deg_pair.first == in_edges.end()) {
                        removeEdges(in_edges, out_edges, i_out_deg_pair, [&](Edge e) { return target(e, DepList); });
                        changed = true;
                        i_out_deg_pair = out_edges.equal_range(*uvi);
                    }

                    // Remove in edges of nodes that don't have a single edge out
                    if (i_out_deg_pair.first == out_edges.end()) {
                        removeEdges(out_edges, in_edges, i_in_deg_pair, [&](Edge e) { return source(e, DepList); });
                        changed = true;
                    }

                    ++uvi;
                }
            }

            // Update colors in graph
            const boost::property_map<Graph, boost::edge_attribute_t>::type& edgeAttrMap = boost::get(boost::edge_attribute, DepList);
            for (auto ei : out_edges)
                edgeAttrMap[ei.second]["color"] = "red";
        }

#if defined(__clang__)
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif

        void markOutOfScopeLinks() {
            const boost::property_map<Graph, boost::edge_attribute_t>::type& edgeAttrMap = boost::get(boost::edge_attribute, DepList);

            for( auto obj : objects) {

                std::vector<App::DocumentObject*> invalids;
                GeoFeatureGroupExtension::getInvalidLinkObjects(obj, invalids);
                //isLinkValid returns true for non-link properties
                for(auto linkedObj : invalids) {

                    auto res = edge(GlobalVertexList[getId(obj)], GlobalVertexList[getId(linkedObj)], DepList);
                    if(res.second)
                        edgeAttrMap[res.first]["color"] = "orange";
                }
            }
        }

        const struct DocumentP* d;
        Graph DepList;
        int vertex_no{0};
        std::map<std::string, Vertex> LocalVertexList;
        std::map<std::string, Vertex> GlobalVertexList;
        std::set<const DocumentObject*> objects;
        std::map<const DocumentObject*, Graph*> GraphList;
        //random color generation
        std::mt19937 seed;
        std::uniform_int_distribution<int> distribution;
    };

    GraphCreator g(d);

    boost::write_graphviz(out, g.getGraph());
}
