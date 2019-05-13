/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


/*! \namespace App \class App::Document
This is besides the Application class the most important class in FreeCAD
It contains all the data of the opened, saved or newly created FreeCAD Document.
The Document manage the Undo and Redo mechanism and the linking of documents.

Note: the documents are not free objects. They are completely handled by the
App::Application. Only the Application can Open or destroy a document.

\section Exception Exception handling
As the document is the main data structure of FreeCAD we have to take a close
look on how Exceptions affect the integrity of the App::Document.

\section UndoRedo Undo Redo an Transactions
Undo Redo handling is one of the major mechanism of an document in terms of
user friendliness and speed (no one will wait for Undo too long).

\section Dependency Graph and dependency handling
The FreeCAD document handles the dependencies of its DocumentObjects with
an adjacence list. This gives the opportunity to calculate the shortest
recompute path. Also enables more complicated dependencies beyond trees.


@see App::Application
@see App::DocumentObject
*/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <sstream>
# include <climits>
# include <bitset>
# include <random>
#endif

#include <boost/algorithm/string.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/bimap.hpp>
#include <boost/graph/strong_components.hpp>

#ifdef USE_OLD_DAG
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>
#endif //USE_OLD_DAG

#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <unordered_set>
#include <unordered_map>
#include <random>

#include <QMap>
#include <QCoreApplication>
#include <QCryptographicHash>

#include "Document.h"
#include "Application.h"
#include "DocumentObject.h"
#include "MergeDocuments.h"
#include "ExpressionParser.h"
#include <App/DocumentPy.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Base/Uuid.h>
#include <Base/Sequencer.h>

#ifdef _MSC_VER
#include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>

#include "Application.h"
#include "Transactions.h"
#include "GeoFeatureGroupExtension.h"
#include "Origin.h"
#include "OriginGroupExtension.h"
#include "Link.h"
#include "GeoFeature.h"

FC_LOG_LEVEL_INIT("App", true, true, true);

using Base::Console;
using Base::streq;
using Base::Writer;
using namespace App;
using namespace std;
using namespace boost;
using namespace zipios;

#if FC_DEBUG
#  define FC_LOGFEATUREUPDATE
#endif

// typedef boost::property<boost::vertex_root_t, DocumentObject* > VertexProperty;
typedef boost::adjacency_list <
boost::vecS,           // class OutEdgeListS  : a Sequence or an AssociativeContainer
boost::vecS,           // class VertexListS   : a Sequence or a RandomAccessContainer
boost::directedS,      // class DirectedS     : This is a directed graph
boost::no_property,    // class VertexProperty:
boost::no_property,    // class EdgeProperty:
boost::no_property,    // class GraphProperty:
boost::listS           // class EdgeListS:
> DependencyList;
typedef boost::graph_traits<DependencyList> Traits;
typedef Traits::vertex_descriptor Vertex;
typedef Traits::edge_descriptor Edge;
typedef std::vector <size_t> Node;
typedef std::vector <size_t> Path;

namespace App {

typedef boost::bimap<StringHasherRef,int> HasherMap;

static bool _IsRestoring;
static bool _IsRelabeling;

// Pimpl class
struct DocumentP
{
    // Array to preserve the creation order of created objects
    std::vector<DocumentObject*> objectArray;
    std::unordered_set<App::DocumentObject*> touchedObjs;
    std::unordered_map<std::string,DocumentObject*> objectMap;
    std::unordered_map<long,DocumentObject*> objectIdMap;
    std::unordered_map<std::string, bool> partialLoadObjects;
    long lastObjectId;
    DocumentObject* activeObject;
    Transaction *activeUndoTransaction;
    int iTransactionMode;
    bool rollback;
    bool undoing; ///< document in the middle of undo or redo
    std::bitset<32> StatusBits;
    int iUndoMode;
    unsigned int UndoMemSize;
    unsigned int UndoMaxStackSize;
    mutable HasherMap hashers;
#ifdef USE_OLD_DAG
    DependencyList DepList;
    std::map<DocumentObject*,Vertex> VertexObjectList;
    std::map<Vertex,DocumentObject*> vertexMap;
#endif //USE_OLD_DAG
    std::multimap<const App::DocumentObject*, 
        std::unique_ptr<App::DocumentObjectExecReturn> > _RecomputeLog;

    DocumentP() {
        static std::random_device _RD;
        static std::mt19937 _RGEN(_RD());
        static std::uniform_int_distribution<> _RDIST(0,5000);
        // Set some random offset to reduce likelyhood of ID collison when
        // copying shape from other document. It is probably better to randomize
        // on each object ID.
        lastObjectId = _RDIST(_RGEN); 
        activeObject = 0;
        activeUndoTransaction = 0;
        iTransactionMode = 0;
        rollback = false;
        undoing = false;
        StatusBits.set((size_t)Document::Closable, true);
        StatusBits.set((size_t)Document::KeepTrailingDigits, true);
        StatusBits.set((size_t)Document::Restoring, false);
        iUndoMode = 0;
        UndoMemSize = 0;
        UndoMaxStackSize = 20;
    }

    void addRecomputeLog(const char *why, App::DocumentObject *obj) {
        addRecomputeLog(new DocumentObjectExecReturn(why,obj));
    }

    void addRecomputeLog(const std::string &why, App::DocumentObject *obj) {
        addRecomputeLog(new DocumentObjectExecReturn(why,obj));
    }

    void addRecomputeLog(DocumentObjectExecReturn *returnCode) {
        if(!returnCode->Which) {
            delete returnCode;
            return;
        }
        _RecomputeLog.emplace(returnCode->Which, std::unique_ptr<DocumentObjectExecReturn>(returnCode));
        returnCode->Which->setStatus(ObjectStatus::Error,true);
    }

    void clearRecomputeLog(const App::DocumentObject *obj=0) {
        if(!obj)
            _RecomputeLog.clear();
        else
            _RecomputeLog.erase(obj);
    }

    const char *findRecomputeLog(const App::DocumentObject *obj) {
        auto range = _RecomputeLog.equal_range(obj);
        if(range.first == range.second)
            return 0;
        return (--range.second)->second->Why.c_str();
    }

    static
    void findAllPathsAt(const std::vector <Node> &all_nodes, size_t id,
                        std::vector <Path> &all_paths, Path tmp);
    std::vector<App::DocumentObject*>
    topologicalSort(const std::vector<App::DocumentObject*>& objects) const;
    std::vector<App::DocumentObject*>
    static partialTopologicalSort(const std::vector<App::DocumentObject*>& objects);
};

} // namespace App

PROPERTY_SOURCE(App::Document, App::PropertyContainer)

bool Document::testStatus(Status pos) const
{
    return d->StatusBits.test((size_t)pos);
}

void Document::setStatus(Status pos, bool on)
{
    d->StatusBits.set((size_t)pos, on);
}

void Document::writeDependencyGraphViz(std::ostream &out)
{
    //  // caching vertex to DocObject
    //std::map<Vertex,DocumentObject*> VertexMap;
    //for(std::map<DocumentObject*,Vertex>::const_iterator It1= _DepConMap.begin();It1 != _DepConMap.end(); ++It1)
    //  VertexMap[It1->second] = It1->first;

    out << "digraph G {" << endl;
    out << "\tordering=out;" << endl;
    out << "\tnode [shape = box];" << endl;

    for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        out << "\t" << It->first << ";" <<endl;
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2)
            if (*It2)
                out << "\t" << It->first << "->" << (*It2)->getNameInDocument() << ";" <<endl;
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
    out << "}" << endl;
}

void Document::exportGraphviz(std::ostream& out) const
{
    /* Typedefs for a graph with graphviz attributes */
    typedef std::map<std::string, std::string> GraphvizAttributes;
    typedef boost::subgraph< adjacency_list<vecS, vecS, directedS,
            property<vertex_attribute_t, GraphvizAttributes>,
            property<edge_index_t, int, property<edge_attribute_t, GraphvizAttributes> >,
            property<graph_name_t, std::string,
            property<graph_graph_attribute_t,  GraphvizAttributes,
            property<graph_vertex_attribute_t, GraphvizAttributes,
            property<graph_edge_attribute_t,   GraphvizAttributes>
            > > > > > Graph;

    /**
     * @brief The GraphCreator class
     *
     * This class creates the dependency graph for a document.
     *
     */

    class GraphCreator {
    public:

        GraphCreator(struct DocumentP* _d) : d(_d), vertex_no(0), seed(std::random_device()()), distribution(0,255) {
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
                return std::string();

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

                // Create subgraphs for all documentobjects that it depends on; it will depend on some property there
                auto i = expressions.begin();
                while (i != expressions.end()) {
                    std::set<ObjectIdentifier> deps;

                    i->second->getIdentifiers(deps);

                    std::set<ObjectIdentifier>::const_iterator j = deps.begin();
                    while (j != deps.end()) {
                        DocumentObject * o = j->getDocumentObject();

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
                        ++j;
                    }
                    ++i;
                }
            }
        }

        /**
         * @brief add Add @docObj to the graph, including all expressions (and dependencies) it includes.
         * @param docObj The document object to add.
         * @param name Name of node.
         */

        void add(DocumentObject * docObj, const std::string & name, const std::string & label, bool CSSubgraphs) {

            //don't add objects twice
            if(std::find(objects.begin(), objects.end(), docObj) != objects.end())
                return;

            //find the correct graph to add the vertex to. Check first expression graphs, afterwards
            //the parent CS and origin graphs
            Graph * sgraph = GraphList[docObj];
            if(CSSubgraphs) {
                if(!sgraph) {
                    auto group = GeoFeatureGroupExtension::getGroupOfObject(docObj);
                    if(group) {
                        if(docObj->isDerivedFrom(App::OriginFeature::getClassTypeId()))
                            sgraph = GraphList[group->getExtensionByType<OriginGroupExtension>()->Origin.getValue()];
                        else
                            sgraph = GraphList[group];
                    }
                }
                if(!sgraph) {
                    if(docObj->isDerivedFrom(OriginFeature::getClassTypeId()))
                        sgraph = GraphList[static_cast<OriginFeature*>(docObj)->getOrigin()];
                }
            }
            if(!sgraph)
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
            auto expressions = docObj->ExpressionEngine.getExpressions();
            auto i = expressions.begin();

            // Add nodes for each property that has an expression attached to it
            while (i != expressions.end()) {
                std::map<std::string, Vertex>::const_iterator k = GlobalVertexList.find(getId(i->first));
                if (k == GlobalVertexList.end()) {
                    int vid = LocalVertexList[getId(i->first)] = add_vertex(*sgraph);
                    GlobalVertexList[getId(i->first)] = vertex_no++;
                    setPropertyVertexAttributes(*sgraph, vid, i->first.toString());
                }

                ++i;
            }

            // Add all dependencies
            i = expressions.begin();
            while (i != expressions.end()) {

                // Get dependencies
                std::set<ObjectIdentifier> deps;
                i->second->getIdentifiers(deps);

                // Create subgraphs for all documentobjects that it depends on; it will depend on some property there
                std::set<ObjectIdentifier>::const_iterator j = deps.begin();
                while (j != deps.end()) {
                    DocumentObject * depObjDoc = j->getDocumentObject();
                    std::map<std::string, Vertex>::const_iterator k = GlobalVertexList.find(getId(*j));

                    if (k == GlobalVertexList.end()) {
                        Graph * depSgraph = GraphList[depObjDoc] ? GraphList[depObjDoc] : &DepList;

                        LocalVertexList[getId(*j)] = add_vertex(*depSgraph);
                        GlobalVertexList[getId(*j)] = vertex_no++;
                        setPropertyVertexAttributes(*depSgraph, LocalVertexList[getId(*j)], j->getPropertyName() + j->getSubPathStr());
                    }

                    ++j;
                }
                ++i;
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
                    if (objectIt->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
                        recursiveCSSubgraphs(objectIt, nullptr);
                }
            }

            // Internal document objects
            for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It)
                addExpressionSubgraphIfNeeded(It->second, CSSubgraphs);

            // Add external document objects
            for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It) {
                std::vector<DocumentObject*> OutList = It->second->getOutList();
                for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
                    if (*It2) {
                        std::map<std::string,Vertex>::const_iterator item = GlobalVertexList.find(getId(*It2));

                        if (item == GlobalVertexList.end())
                            addExpressionSubgraphIfNeeded(*It2, CSSubgraphs);
                    }
                }
            }

        }

        // Filling up the adjacency List
        void buildAdjacencyList() {

            ParameterGrp::handle depGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DependencyGraph");
            bool CSSubgraphs = depGrp->GetBool("GeoFeatureSubgraphs", true);

            // Add internal document objects
            for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It)
                add(It->second, It->second->getNameInDocument(), It->second->Label.getValue(), CSSubgraphs);

            // Add external document objects
            for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It) {
                std::vector<DocumentObject*> OutList = It->second->getOutList();
                for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
                    if (*It2) {
                        std::map<std::string,Vertex>::const_iterator item = GlobalVertexList.find(getId(*It2));

                        if (item == GlobalVertexList.end())
                            add(*It2,
                                std::string((*It2)->getDocument()->getName()) + "#" + (*It2)->getNameInDocument(),
                                std::string((*It2)->getDocument()->getName()) + "#" + (*It2)->Label.getValue(),
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
            std::set<const DocumentObject*>::const_iterator j = objects.begin();
            while (j != objects.end()) {
                const DocumentObject * docObj = *j;

                // Add expressions and its dependencies
                auto expressions = docObj->ExpressionEngine.getExpressions();
                auto i = expressions.begin();

                while (i != expressions.end()) {
                    std::set<ObjectIdentifier> deps;
                    i->second->getIdentifiers(deps);

                    // Create subgraphs for all documentobjects that it depends on; it will depend on some property there
                    std::set<ObjectIdentifier>::const_iterator k = deps.begin();
                    while (k != deps.end()) {
                        DocumentObject * depObjDoc = k->getDocumentObject();
                        Edge edge;
                        bool inserted;

                        tie(edge, inserted) = add_edge(GlobalVertexList[getId(i->first)], GlobalVertexList[getId(*k)], DepList);

                        // Add this edge to the set of all expression generated edges
                        existingEdges.insert(std::make_pair(docObj, depObjDoc));

                        // Edges between properties should be a bit smaller, and dashed
                        edgeAttrMap[edge]["arrowsize"] = "0.5";
                        edgeAttrMap[edge]["style"] = "dashed";
                        ++k;
                    }
                    ++i;
                }
                ++j;
            }

            ParameterGrp::handle depGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DependencyGraph");
            bool omitGeoFeatureGroups = depGrp->GetBool("GeoFeatureSubgraphs", true);

            // Add edges between document objects
            for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It) {

                if(omitGeoFeatureGroups) {
                    //coordinate systems are represented by subgraphs
                    if(It->second->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
                        continue;

                    //as well as origins
                    if(It->second->isDerivedFrom(Origin::getClassTypeId()))
                        continue;
                }

                std::map<DocumentObject*, int> dups;
                std::vector<DocumentObject*> OutList = It->second->getOutList();
                const DocumentObject * docObj = It->second;

                for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
                    if (*It2) {

                        // Count duplicate edges
                        bool inserted = edge(GlobalVertexList[getId(docObj)], GlobalVertexList[getId(*It2)], DepList).second;
                        if (inserted) {
                            dups[*It2]++;
                            continue;
                        }

                        // Skip edge if an expression edge already exists
                        if (existingEdges.find(std::make_pair(docObj, *It2)) != existingEdges.end())
                            continue;

                        // Add edge

                        Edge edge;

                        tie(edge, inserted) = add_edge(GlobalVertexList[getId(docObj)], GlobalVertexList[getId(*It2)], DepList);

                        // Set properties to make arrows go between subgraphs if needed
                        if (GraphList[docObj])
                            edgeAttrMap[edge]["ltail"] = getClusterName(docObj);
                        if (GraphList[*It2])
                            edgeAttrMap[edge]["lhead"] = getClusterName(*It2);
                    }
                }

                // Set labels for duplicate edges
                for (std::map<DocumentObject*, int>::const_iterator It2 = dups.begin(); It2 != dups.end(); ++It2) {
                    Edge e(edge(GlobalVertexList[getId(It->second)], GlobalVertexList[getId(It2->first)], DepList).first);
                    std::stringstream s;
                    s << " " << (It2->second + 1) << "x";
                    edgeAttrMap[e]["label"] = s.str();
                }

            }

        }

        typedef std::unordered_multimap<Vertex, Edge> EdgeMap;

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
                in_edges.insert(std::make_pair<Vertex, Edge>(target(*ei, DepList), *ei));
                out_edges.insert(std::make_pair<Vertex, Edge>(source(*ei, DepList), *ei));
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
            for (auto ei = out_edges.begin(), ei_end = out_edges.end(); ei != ei_end; ++ei)
                edgeAttrMap[ei->second]["color"] = "red";
        }

        void markOutOfScopeLinks() {
            const boost::property_map<Graph, boost::edge_attribute_t>::type& edgeAttrMap = boost::get(boost::edge_attribute, DepList);

            for( auto obj : objects) {

                std::vector<App::DocumentObject*> invalids;
                GeoFeatureGroupExtension::getInvalidLinkObjects(obj, invalids);
                //isLinkValid returns true for non-link properties
                for(auto linkedObj : invalids) {

                    auto res = edge(GlobalVertexList[getId(obj)], GlobalVertexList[getId(linkedObj)], DepList);
                    if(res.second)
                        edgeAttrMap[res.first]["color"] = "red";
                }
            }
        }

        const struct DocumentP* d;
        Graph DepList;
        int vertex_no;
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

//bool _has_cycle_dfs(const DependencyList & g, vertex_t u, default_color_type * color)
//{
//  color[u] = gray_color;
//  graph_traits < DependencyList >::adjacency_iterator vi, vi_end;
//  for (tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
//    if (color[*vi] == white_color)
//      if (has_cycle_dfs(g, *vi, color))
//        return true;            // cycle detected, return immediately
//      else if (color[*vi] == gray_color)        // *vi is an ancestor!
//        return true;
//  color[u] = black_color;
//  return false;
//}

bool Document::checkOnCycle(void)
{/*
  std::vector < default_color_type > color(num_vertices(_DepList), white_color);
  graph_traits < DependencyList >::vertex_iterator vi, vi_end;
  for (tie(vi, vi_end) = vertices(_DepList); vi != vi_end; ++vi)
    if (color[*vi] == white_color)
      if (_has_cycle_dfs(_DepList, *vi, &color[0]))
        return true; */
    return false;
}

bool Document::undo(int id)
{
    if (d->iUndoMode) {
        if(id) {
            auto it = mUndoMap.find(id);
            if(it == mUndoMap.end())
                return false;
            if(it->second != d->activeUndoTransaction) {
                while(mUndoTransactions.size() && mUndoTransactions.back()!=it->second)
                    undo(0);
            }
        }

        if (d->activeUndoTransaction)
            commitTransaction();
        if (mUndoTransactions.empty())
            return false;
        // redo
        d->activeUndoTransaction = new Transaction(mUndoTransactions.back()->getID());
        d->activeUndoTransaction->Name = mUndoTransactions.back()->Name;

        Base::FlagToggler<bool> flag(d->undoing);
        // applying the undo
        mUndoTransactions.back()->apply(*this,false);

        // save the redo
        mRedoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
        mRedoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;

        mUndoMap.erase(mUndoTransactions.back()->getID());
        delete mUndoTransactions.back();
        mUndoTransactions.pop_back();

        signalUndo(*this);
        return true;
    }

    return false;
}

bool Document::redo(int id)
{
    if (d->iUndoMode) {
        if(id) {
            auto it = mRedoMap.find(id);
            if(it == mRedoMap.end())
                return false;
            while(mRedoTransactions.size() && mRedoTransactions.back()!=it->second)
                redo(0);
        }

        if (d->activeUndoTransaction)
            commitTransaction();

        assert(mRedoTransactions.size()!=0);

        // undo
        d->activeUndoTransaction = new Transaction(mRedoTransactions.back()->getID());
        d->activeUndoTransaction->Name = mRedoTransactions.back()->Name;

        // do the redo
        Base::FlagToggler<bool> flag(d->undoing);
        mRedoTransactions.back()->apply(*this,true);

        mUndoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
        mUndoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;

        mRedoMap.erase(mRedoTransactions.back()->getID());
        delete mRedoTransactions.back();
        mRedoTransactions.pop_back();

        signalRedo(*this);
        return true;
    }

    return false;
}

void Document::removePropertyOfObject(TransactionalObject* obj, const char* name)
{
    Property* prop = obj->getDynamicPropertyByName(name);
    if (prop) {
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->removeProperty(obj, prop);
        for (auto it : mUndoTransactions)
            it->removeProperty(obj, prop);
        for (auto it : mRedoTransactions)
            it->removeProperty(obj, prop);
    }
}

bool Document::isPerformingTransaction() const
{
    return d->undoing || d->rollback;
}

std::vector<std::string> Document::getAvailableUndoNames() const
{
    std::vector<std::string> vList;
    if (d->activeUndoTransaction)
        vList.push_back(d->activeUndoTransaction->Name);
    for (std::list<Transaction*>::const_reverse_iterator It=mUndoTransactions.rbegin();It!=mUndoTransactions.rend();++It)
        vList.push_back((**It).Name);
    return vList;
}

std::vector<std::string> Document::getAvailableRedoNames() const
{
    std::vector<std::string> vList;
    for (std::list<Transaction*>::const_reverse_iterator It=mRedoTransactions.rbegin();It!=mRedoTransactions.rend();++It)
        vList.push_back((**It).Name);
    return vList;
}

void Document::openTransaction(const char* name) {
    if(isPerformingTransaction()) {
        FC_WARN("Cannot open transaction while transacting");
        return;
    }

    auto &app = GetApplication();
    if(app.autoTransaction())
        app.setActiveTransaction(name?name:"<empty>");
    else
        _openTransaction(name);
}

int Document::_openTransaction(const char* name, int id)
{
    if(isPerformingTransaction()) {
        FC_WARN("Cannot open transaction while transacting");
        return 0;
    }

    if (d->iUndoMode) {
        if(id && mUndoMap.find(id)!=mUndoMap.end())
            throw Base::RuntimeError("invalid transaction id");
        if (d->activeUndoTransaction)
            commitTransaction();
        _clearRedos();

        d->activeUndoTransaction = new Transaction(id);
        if (!name)
            name = "<empty>";
        d->activeUndoTransaction->Name = name;
        mUndoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
        id = d->activeUndoTransaction->getID();

        signalOpenTransaction(*this, name);

        auto &app = GetApplication();
        auto activeDoc = app.getActiveDocument();
        if(activeDoc && 
           activeDoc!=this && 
           app.autoTransaction() && 
           !activeDoc->hasPendingTransaction()) 
        {
            std::string aname("-> ");
            aname += d->activeUndoTransaction->Name;
            FC_LOG("auto transaction " << getName() << " -> " << activeDoc->getName());
            activeDoc->_openTransaction(aname.c_str(),id);
        }
        return id;
    }
    return 0;
}

void Document::_checkTransaction(DocumentObject* pcDelObj, const Property *What, int line)
{
    // if the undo is active but no transaction open, open one!
    if (d->iUndoMode && !isPerformingTransaction()) {
        if (!d->activeUndoTransaction) {
            if(!testStatus(Restoring) || testStatus(Importing)) {
                int tid=0;
                const char *name = GetApplication().getActiveTransaction(&tid);
                if(name && tid>0) {
                    bool ignore = false;
                    if(What) {
                        auto parent = What->getContainer();
                        auto parentObj = Base::freecad_dynamic_cast<DocumentObject>(parent);
                        if(!parentObj || What->testStatus(Property::NoModify))
                            ignore = true;
                    }
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                        if(What) {
                            auto parent = What->getContainer();
                            auto obj = dynamic_cast<DocumentObject*>(parent);
                            const char *objName = obj?obj->getNameInDocument():0;
                            const char *propName = What->getName();
                            FC_LOG((ignore?"ignore":"auto") << " transaction (" 
                                    << line << ") '" << name 
                                    << "' on change of " << getName() << '.'
                                    << (objName?objName:"<?>") << '.' 
                                    << (propName?propName:"<?>"));
                        }else
                            FC_LOG((ignore?"ignore":"auto") <<" transaction (" 
                                    << line << ") '" << name << "' in " << getName());
                    }
                    if(!ignore)
                        _openTransaction(name,tid);
                    return;
                }
            }
            if(!pcDelObj) return;
            // When the object is going to be deleted we have to check if it has already been added to
            // the undo transactions
            std::list<Transaction*>::iterator it;
            for (it = mUndoTransactions.begin(); it != mUndoTransactions.end(); ++it) {
                if ((*it)->hasObject(pcDelObj)) {
                    _openTransaction("Delete");
                    break;
                }
            }
        }
    }
}

void Document::_clearRedos()
{
    if(isPerformingTransaction()) {
        FC_ERR("Cannot clear redo while transacting");
        return;
    }

    mRedoMap.clear();
    while (!mRedoTransactions.empty()) {
        delete mRedoTransactions.back();
        mRedoTransactions.pop_back();
    }
}

void Document::commitTransaction()
{
    if(isPerformingTransaction()) {
        FC_WARN("Cannot commit transaction while transacting");
        return;
    }

    if (d->activeUndoTransaction) {
        int id = d->activeUndoTransaction->getID();
        mUndoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;
        // check the stack for the limits
        if(mUndoTransactions.size() > d->UndoMaxStackSize){
            mUndoMap.erase(mUndoTransactions.front()->getID());
            delete mUndoTransactions.front();
            mUndoTransactions.pop_front();
        }
        signalCommitTransaction(*this);
        GetApplication().closeActiveTransaction(false,id);
    }else if(GetApplication().autoTransaction())
        GetApplication().closeActiveTransaction(false);
}

void Document::abortTransaction()
{
    if(isPerformingTransaction()) {
        FC_WARN("Cannot abort transaction while transacting");
        return;
    }

    if (d->activeUndoTransaction) {
        {
            Base::FlagToggler<bool> flag(d->rollback);
            // applying the so far made changes
            d->activeUndoTransaction->apply(*this,false);
        }

        // destroy the undo
        int id = d->activeUndoTransaction->getID();
        mUndoMap.erase(d->activeUndoTransaction->getID());
        delete d->activeUndoTransaction;
        d->activeUndoTransaction = 0;
        signalAbortTransaction(*this);
        GetApplication().closeActiveTransaction(true,id);
    }else if(GetApplication().autoTransaction())
        GetApplication().closeActiveTransaction(true);
}

bool Document::hasPendingTransaction() const
{
    if (d->activeUndoTransaction)
        return true;
    else
        return false;
}

int Document::getTransactionID(bool undo, unsigned pos) const {
    if(undo) {
        if(d->activeUndoTransaction) {
            if(pos == 0)
                return d->activeUndoTransaction->getID();
            --pos;
        }
        if(pos>=mUndoTransactions.size())
            return 0;
        auto rit = mUndoTransactions.rbegin();
        for(;pos;++rit,--pos);
        return (*rit)->getID();
    }
    if(pos>=mRedoTransactions.size())
        return 0;
    auto rit = mRedoTransactions.rbegin();
    for(;pos;++rit,--pos);
    return (*rit)->getID();
}

bool Document::isTransactionEmpty() const
{
    if (d->activeUndoTransaction) {
        return d->activeUndoTransaction->isEmpty();
    }

    return true;
}

void Document::clearUndos()
{
    if(isPerformingTransaction()) {
        FC_ERR("Cannot clear undos while transacting");
        return;
    }

    if (d->activeUndoTransaction)
        commitTransaction();

    mUndoMap.clear();

    // When cleaning up the undo stack we must delete the transactions from front
    // to back because a document object can appear in several transactions but
    // once removed from the document the object can never ever appear in any later
    // transaction. Since the document object may be also deleted when the transaction
    // is deleted we must make sure not access an object once it's destroyed. Thus, we
    // go from front to back and not the other way round.
    while (!mUndoTransactions.empty()) {
        delete mUndoTransactions.front();
        mUndoTransactions.pop_front();
    }
    //while (!mUndoTransactions.empty()) {
    //    delete mUndoTransactions.back();
    //    mUndoTransactions.pop_back();
    //}

    _clearRedos();
}

int Document::getAvailableUndos(int id) const
{
    if(id) {
        auto it = mUndoMap.find(id);
        if(it == mUndoMap.end())
            return 0;
        int i = 0;
        if(d->activeUndoTransaction) {
            ++i;
            if(d->activeUndoTransaction->getID()==id)
                return i;
        }
        auto rit = mUndoTransactions.rbegin();
        for(;rit!=mUndoTransactions.rend()&&*rit!=it->second;++rit,++i);
        assert(rit!=mUndoTransactions.rend());
        return i+1;
    }
    if (d->activeUndoTransaction)
        return static_cast<int>(mUndoTransactions.size() + 1);
    else
        return static_cast<int>(mUndoTransactions.size());
}

int Document::getAvailableRedos(int id) const
{
    if(id) {
        auto it = mRedoMap.find(id);
        if(it == mRedoMap.end())
            return 0;
        int i = 0;
        for(auto rit=mRedoTransactions.rbegin();*rit!=it->second;++rit,++i);
        assert(i<(int)mRedoTransactions.size());
        return i+1;
    }
    return static_cast<int>(mRedoTransactions.size());
}

void Document::setUndoMode(int iMode)
{
    if (d->iUndoMode && !iMode)
        clearUndos();

    d->iUndoMode = iMode;
}

int Document::getUndoMode(void) const
{
    return d->iUndoMode;
}

unsigned int Document::getUndoMemSize (void) const
{
    return d->UndoMemSize;
}

void Document::setUndoLimit(unsigned int UndoMemSize)
{
    d->UndoMemSize = UndoMemSize;
}

void Document::setMaxUndoStackSize(unsigned int UndoMaxStackSize)
{
     d->UndoMaxStackSize = UndoMaxStackSize;
}

unsigned int Document::getMaxUndoStackSize(void)const
{
    return d->UndoMaxStackSize;
}

void Document::onBeforeChange(const Property* prop)
{
    if(prop == &Label)
        oldLabel = Label.getValue();
    signalBeforeChange(*this, *prop);
}

void Document::onChanged(const Property* prop)
{
    signalChanged(*this, *prop);

    // the Name property is a label for display purposes
    if (prop == &Label) {
        Base::FlagToggler<> flag(_IsRelabeling);
        App::GetApplication().signalRelabelDocument(*this);
    } else if(prop == &ShowHidden) {
        App::GetApplication().signalShowHidden(*this);
    } else if (prop == &Uid) {
        std::string new_dir = getTransientDirectoryName(this->Uid.getValueStr(),this->FileName.getStrValue());
        std::string old_dir = this->TransientDir.getStrValue();
        Base::FileInfo TransDirNew(new_dir);
        Base::FileInfo TransDirOld(old_dir);
        // this directory should not exist
        if (!TransDirNew.exists()) {
            if (TransDirOld.exists()) {
                if (!TransDirOld.renameFile(new_dir.c_str()))
                    Base::Console().Warning("Failed to rename '%s' to '%s'\n", old_dir.c_str(), new_dir.c_str());
                else
                    this->TransientDir.setValue(new_dir);
            }
            else {
                if (!TransDirNew.createDirectory())
                    Base::Console().Warning("Failed to create '%s'\n", new_dir.c_str());
                else
                    this->TransientDir.setValue(new_dir);
            }
        }
        // when reloading an existing document the transient directory doesn't change
        // so we must avoid to generate a new uuid
        else if (TransDirNew.filePath() != TransDirOld.filePath()) {
            // make sure that the uuid is unique
            std::string uuid = this->Uid.getValueStr();
            Base::Uuid id;
            Base::Console().Warning("Document with the UUID '%s' already exists, change to '%s'\n",
                                    uuid.c_str(), id.getValue().c_str());
            // recursive call of onChanged()
            this->Uid.setValue(id);
        }
    } else if(prop == &UseHasher) {
        for(auto obj : d->objectArray) {
            auto geofeature = dynamic_cast<GeoFeature*>(obj);
            if(geofeature && geofeature->getPropertyOfGeometry())
                geofeature->enforceRecompute();
        }
    }
}

void Document::onBeforeChangeProperty(const TransactionalObject *Who, const Property *What)
{
    if(Who->isDerivedFrom(App::DocumentObject::getClassTypeId()))
        signalBeforeChangeObject(*static_cast<const App::DocumentObject*>(Who), *What);
    if(!d->rollback && !_IsRelabeling) {
        _checkTransaction(0,What,__LINE__);
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectChange(Who,What);
    }
}

void Document::onChangedProperty(const DocumentObject *Who, const Property *What)
{
    signalChangedObject(*Who, *What);
}

void Document::setTransactionMode(int iMode)
{
    d->iTransactionMode = iMode;
}

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
Document::Document(void)
    :Hasher(new StringHasher)
{
    // Remark: In a constructor we should never increment a Python object as we cannot be sure
    // if the Python interpreter gets a reference of it. E.g. if we increment but Python don't
    // get a reference then the object wouldn't get deleted in the destructor.
    // So, we must increment only if the interpreter gets a reference.
    // Remark: We force the document Python object to own the DocumentPy instance, thus we don't
    // have to care about ref counting any more.
    DocumentPythonObject = Py::Object(new DocumentPy(this), true);
    d = new DocumentP;

#ifdef FC_LOGUPDATECHAIN
    Console().Log("+App::Document: %p\n",this);
#endif
    std::string CreationDateString = Base::TimeInfo::currentDateTimeString();
    std::string Author = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetASCII("prefAuthor","");
    std::string AuthorComp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetASCII("prefCompany","");
    ADD_PROPERTY_TYPE(Label,("Unnamed"),0,Prop_None,"The name of the document");
    ADD_PROPERTY_TYPE(FileName,(""),0,PropertyType(Prop_Transient|Prop_ReadOnly),"The path to the file where the document is saved to");
    ADD_PROPERTY_TYPE(CreatedBy,(Author.c_str()),0,Prop_None,"The creator of the document");
    ADD_PROPERTY_TYPE(CreationDate,(CreationDateString.c_str()),0,Prop_ReadOnly,"Date of creation");
    ADD_PROPERTY_TYPE(LastModifiedBy,(""),0,Prop_None,0);
    ADD_PROPERTY_TYPE(LastModifiedDate,("Unknown"),0,Prop_ReadOnly,"Date of last modification");
    ADD_PROPERTY_TYPE(Company,(AuthorComp.c_str()),0,Prop_None,"Additional tag to save the name of the company");
    ADD_PROPERTY_TYPE(Comment,(""),0,Prop_None,"Additional tag to save a comment");
    ADD_PROPERTY_TYPE(Meta,(),0,Prop_None,"Map with additional meta information");
    ADD_PROPERTY_TYPE(Material,(),0,Prop_None,"Map with material properties");
    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id,(""),0,Prop_None,"ID of the document");
    ADD_PROPERTY_TYPE(Uid,(id),0,Prop_ReadOnly,"UUID of the document");

    // license stuff
    ADD_PROPERTY_TYPE(License,("CC-BY 3.0"),0,Prop_None,"License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,("http://creativecommons.org/licenses/by/3.0/"),0,Prop_None,"URL to the license text/contract");

    // license stuff
    int licenseId = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetInt("prefLicenseType",0);
    std::string license;
    std::string licenseUrl;
    switch (licenseId) {
        case 0:
            license = "All rights reserved";
            licenseUrl = "http://en.wikipedia.org/wiki/All_rights_reserved";
            break;
        case 1:
            license = "CreativeCommons Attribution";
            licenseUrl = "http://creativecommons.org/licenses/by/4.0/";
            break;
        case 2:
            license = "CreativeCommons Attribution-ShareAlike";
            licenseUrl = "http://creativecommons.org/licenses/by-sa/4.0/";
            break;
        case 3:
            license = "CreativeCommons Attribution-NoDerivatives";
            licenseUrl = "http://creativecommons.org/licenses/by-nd/4.0/";
            break;
        case 4:
            license = "CreativeCommons Attribution-NonCommercial";
            licenseUrl = "http://creativecommons.org/licenses/by-nc/4.0/";
            break;
        case 5:
            license = "CreativeCommons Attribution-NonCommercial-ShareAlike";
            licenseUrl = "http://creativecommons.org/licenses/by-nc-sa/4.0/";
            break;
        case 6:
            license = "CreativeCommons Attribution-NonCommercial-NoDerivatives";
            licenseUrl = "http://creativecommons.org/licenses/by-nc-nd/4.0/";
            break;
        case 7:
            license = "Public Domain";
            licenseUrl = "http://en.wikipedia.org/wiki/Public_domain";
            break;
        case 8:
            license = "FreeArt";
            licenseUrl = "http://artlibre.org/licence/lal";
            break;
        default:
            license = "Other";
            break;
    }

    licenseUrl = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetASCII("prefLicenseUrl", licenseUrl.c_str());

    ADD_PROPERTY_TYPE(License,(license.c_str()),0,Prop_None,"License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,(licenseUrl.c_str()),0,Prop_None,"URL to the license text/contract");
    ADD_PROPERTY_TYPE(ShowHidden,(false), 0,PropertyType(Prop_None), 
                        "Whether to show hidden object items in the tree view");
    ADD_PROPERTY_TYPE(UseHasher,(true), 0,PropertyType(Prop_None), 
                        "Whether to use hasher on topological naming");
    if(!App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetBool("UseHasher",true))
        UseHasher.setValue(false);

    // this creates and sets 'TransientDir' in onChanged()
    ADD_PROPERTY_TYPE(TransientDir,(""),0,PropertyType(Prop_Transient|Prop_ReadOnly),
        "Transient directory, where the files live while the document is open");
    ADD_PROPERTY_TYPE(Tip,(0),0,PropertyType(Prop_Transient),
        "Link of the tip object of the document");
    ADD_PROPERTY_TYPE(TipName,(""),0,PropertyType(Prop_Hidden|Prop_ReadOnly),
        "Link of the tip object of the document");
    Uid.touch();
}

Document::~Document()
{
#ifdef FC_LOGUPDATECHAIN
    Console().Log("-App::Document: %s %p\n",getName(), this);
#endif

    try {
        clearUndos();
    }
    catch (const boost::exception&) {
    }

#ifdef FC_LOGUPDATECHAIN
    Console().Log("-Delete Features of %s \n",getName());
#endif

    d->objectArray.clear();
    for (auto it = d->objectMap.begin(); it != d->objectMap.end(); ++it) {
        it->second->setStatus(ObjectStatus::Destroy, true);
        delete(it->second);
    }

    // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
    // Python object or not. In the constructor we forced the wrapper to own the object so we need
    // not to dec'ref the Python object any more.
    // But we must still invalidate the Python object because it doesn't need to be
    // destructed right now because the interpreter can own several references to it.
    Base::PyGILStateLocker lock;
    Base::PyObjectBase* doc = (Base::PyObjectBase*)DocumentPythonObject.ptr();
    // Call before decrementing the reference counter, otherwise a heap error can occur
    doc->setInvalid();

    // remove Transient directory
    try {
        Base::FileInfo TransDir(TransientDir.getValue());
        TransDir.deleteDirectoryRecursive();
    }
    catch (const Base::Exception& e) {
        std::cerr << "Removing transient directory failed: " << e.what() << std::endl;
    }
    delete d;
}

std::string Document::getTransientDirectoryName(const std::string& uuid, const std::string& filename) const
{
    // Create a directory name of the form: {ExeName}_Doc_{UUID}_{HASH}_{PID}
    std::stringstream s;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(filename.c_str(), filename.size());
    s << App::Application::getTempPath() << GetApplication().getExecutableName()
      << "_Doc_" << uuid
      << "_" << hash.result().toHex().left(6).constData()
      << "_" << QCoreApplication::applicationPid();
    return s.str();
}

//--------------------------------------------------------------------------
// Exported functions
//--------------------------------------------------------------------------

void Document::Save (Base::Writer &writer) const
{
    d->hashers.clear();
    addStringHasher(Hasher);

    writer.Stream() << "<Document SchemaVersion=\"4\" ProgramVersion=\""
                    << App::Application::Config()["BuildVersionMajor"] << "."
                    << App::Application::Config()["BuildVersionMinor"] << "R"
                    << App::Application::Config()["BuildRevision"]
                    << "\" FileVersion=\"" << writer.getFileVersion() 
                    << "\" StringHasher=\"1\">" << endl;
    
    Hasher->Save(writer);

    PropertyContainer::Save(writer);

    // writing the features types
    writeObjects(d->objectArray, writer);
    writer.Stream() << "</Document>" << endl;

    d->hashers.clear();
}

void Document::Restore(Base::XMLReader &reader)
{
    int i,Cnt;
    d->hashers.clear();
    d->touchedObjs.clear();
    addStringHasher(Hasher);

    setStatus(Document::PartialDoc,false);

    reader.readElement("Document");
    long scheme = reader.getAttributeAsInteger("SchemaVersion");
    reader.DocumentSchema = scheme;
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute("ProgramVersion");
    } else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = reader.getAttributeAsUnsigned("FileVersion");
    } else {
        reader.FileVersion = 0;
    }

    if (reader.hasAttribute("StringHasher"))
        Hasher->Restore(reader);
    else
        Hasher->clear();

    // When this document was created the FileName and Label properties
    // were set to the absolute path or file name, respectively. To save
    // the document to the file it was loaded from or to show the file name
    // in the tree view we must restore them after loading the file because
    // they will be overridden.
    // Note: This does not affect the internal name of the document in any way
    // that is kept in Application.
    std::string FilePath = FileName.getValue();
    std::string DocLabel = Label.getValue();

    // read the Document Properties, when reading in Uid the transient directory gets renamed automatically
    PropertyContainer::Restore(reader);

    // We must restore the correct 'FileName' property again because the stored
    // value could be invalid.
    FileName.setValue(FilePath.c_str());
    Label.setValue(DocLabel.c_str());

    // SchemeVersion "2"
    if ( scheme == 2 ) {
        // read the feature types
        reader.readElement("Features");
        Cnt = reader.getAttributeAsInteger("Count");
        for (i=0 ;i<Cnt ;i++) {
            reader.readElement("Feature");
            string type = reader.getAttribute("type");
            string name = reader.getAttribute("name");
            try {
                addObject(type.c_str(), name.c_str(), /*isNew=*/ false);
            }
            catch ( Base::Exception& ) {
                Base::Console().Message("Cannot create object '%s'\n", name.c_str());
            }
        }
        reader.readEndElement("Features");

        // read the features itself
        reader.readElement("FeatureData");
        Cnt = reader.getAttributeAsInteger("Count");
        for (i=0 ;i<Cnt ;i++) {
            reader.readElement("Feature");
            string name = reader.getAttribute("name");
            DocumentObject* pObj = getObject(name.c_str());
            if (pObj) { // check if this feature has been registered
                pObj->setStatus(ObjectStatus::Restore, true);
                pObj->Restore(reader);
                pObj->setStatus(ObjectStatus::Restore, false);
            }
            reader.readEndElement("Feature");
        }
        reader.readEndElement("FeatureData");
    } // SchemeVersion "3" or higher
    else if ( scheme >= 3 ) {
        // read the feature types
        readObjects(reader);

        // tip object handling. First the whole document has to be read, then we
        // can restore the Tip link out of the TipName Property:
        Tip.setValue(getObject(TipName.getValue()));
    }

    reader.readEndElement("Document");
    d->hashers.clear();
}

std::pair<bool,int> Document::addStringHasher(StringHasherRef hasher) const {
    auto ret = d->hashers.left.insert(HasherMap::left_map::value_type(hasher,(int)d->hashers.size()));
    return std::make_pair(ret.second,ret.first->second);
}

StringHasherRef Document::getStringHasher(int idx) const {
    StringHasherRef hasher;
    if(idx<0) {
        if(UseHasher.getValue())
            return Hasher;
        return hasher;
    }

    auto it = d->hashers.right.find(idx);
    if(it == d->hashers.right.end()) {
        hasher = new StringHasher;
        d->hashers.right.insert(HasherMap::right_map::value_type(idx,hasher));
    }else
        hasher = it->second;
    return hasher;
}

struct DocExportStatus {
    Document::ExportStatus status;
    std::set<const App::DocumentObject*> objs;
};

static DocExportStatus _ExportStatus;

// Exception-safe exporting status setter
class DocumentExporting {
public:
    DocumentExporting(const std::vector<App::DocumentObject*> &objs) {
        _ExportStatus.status = Document::Exporting;
        _ExportStatus.objs.insert(objs.begin(),objs.end());
    }

    ~DocumentExporting() {
        _ExportStatus.status = Document::NotExporting;
        _ExportStatus.objs.clear();
    }
};

// The current implementation choose to use a static variable for exporting
// status because we can be exporting multiple objects from multiple documents
// at the same time. I see no benefits in distinguish which documents are
// exporting, so just use a static variable for global status. But the
// implementation can easily be changed here if necessary.
Document::ExportStatus Document::isExporting(const App::DocumentObject *obj) const {
    if(_ExportStatus.status!=Document::NotExporting &&
       (!obj || _ExportStatus.objs.find(obj)!=_ExportStatus.objs.end()))
        return _ExportStatus.status;
    return Document::NotExporting;
}

void Document::exportObjects(const std::vector<App::DocumentObject*>& obj, std::ostream& out) {

    DocumentExporting exporting(obj);
    d->hashers.clear();

    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        for(auto o : obj) {
            if(o && o->getNameInDocument())
                FC_LOG("exporting " << o->getFullName());
        }
    }

    Base::ZipWriter writer(out);
    writer.putNextEntry("Document.xml");
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl;
    writer.Stream() << "<Document SchemaVersion=\"4\" ProgramVersion=\""
                        << App::Application::Config()["BuildVersionMajor"] << "."
                        << App::Application::Config()["BuildVersionMinor"] << "R"
                        << App::Application::Config()["BuildRevision"]
                        << "\" FileVersion=\"1\">" << endl;
    // Add this block to have the same layout as for normal documents
    writer.Stream() << "<Properties Count=\"0\">" << endl;
    writer.Stream() << "</Properties>" << endl;

    // writing the object types
    writeObjects(obj, writer);
    writer.Stream() << "</Document>" << endl;

    // Hook for others to add further data.
    signalExportObjects(obj, writer);

    // write additional files
    writer.writeFiles();
    d->hashers.clear();
}

#define FC_ATTR_DEPENDENCIES "Dependencies"
#define FC_ELEMENT_OBJECT_DEPS "ObjectDeps"
#define FC_ATTR_DEP_COUNT "Count"
#define FC_ATTR_DEP_OBJ_NAME "Name"
#define FC_ATTR_DEP_COUNT "Count"
#define FC_ATTR_DEP_ALLOW_PARTIAL "AllowPartial"
#define FC_ELEMENT_OBJECT_DEP "Dep"

void Document::writeObjects(const std::vector<App::DocumentObject*>& obj,
                            Base::Writer &writer) const
{
    // writing the features types
    writer.incInd(); // indentation for 'Objects count'
    writer.Stream() << writer.ind() << "<Objects Count=\"" << obj.size();
    if(!isExporting(0))
        writer.Stream() << "\" " FC_ATTR_DEPENDENCIES "=\"1";
    writer.Stream() << "\">" << endl;

    writer.incInd(); // indentation for 'Object type'

    if(!isExporting(0)) {
        for(auto o : obj) {
            const auto &outList = o->getOutList(DocumentObject::OutListNoHidden);
            writer.Stream() << writer.ind() 
                << "<" FC_ELEMENT_OBJECT_DEPS " " FC_ATTR_DEP_OBJ_NAME "=\""
                << o->getNameInDocument() << "\" " FC_ATTR_DEP_COUNT "=\"" << outList.size();
            if(outList.empty()) {
                writer.Stream() << "\"/>" << endl;
                continue;
            }
            int partial = o->canLoadPartial();
            if(partial>0)
                writer.Stream() << "\" " FC_ATTR_DEP_ALLOW_PARTIAL << "=\"" << partial;
            writer.Stream() << "\">" << endl;
            writer.incInd();
            for(auto dep : outList) {
                auto name = dep?dep->getNameInDocument():"";
                writer.Stream() << writer.ind() << "<" FC_ELEMENT_OBJECT_DEP " "
                    FC_ATTR_DEP_OBJ_NAME "=\"" << (name?name:"") << "\"/>" << endl;
            }
            writer.decInd();
            writer.Stream() << writer.ind() << "</" FC_ELEMENT_OBJECT_DEPS ">" << endl;
        }
    }

    std::vector<DocumentObject*>::const_iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        writer.Stream() << writer.ind() << "<Object "
        << "type=\"" << (*it)->getTypeId().getName()     << "\" "
        << "name=\"" << (*it)->getExportName()       << "\" "
        << "id=\"" << (*it)->getID()       << "\" "
        << "ViewType=\"" << (*it)->getViewProviderNameStored() << "\" ";

        // See DocumentObjectPy::getState
        if ((*it)->testStatus(ObjectStatus::Touch))
            writer.Stream() << "Touched=\"1\" ";
        if ((*it)->testStatus(ObjectStatus::Error)) {
            writer.Stream() << "Invalid=\"1\" ";
            auto desc = getErrorDescription(*it);
            if(desc) 
                writer.Stream() << "Error=\"" << Property::encodeAttribute(desc) << "\" ";
        }
        writer.Stream() << "/>" << endl;
    }

    writer.decInd();  // indentation for 'Object type'
    writer.Stream() << writer.ind() << "</Objects>" << endl;

    // writing the features itself
    writer.Stream() << writer.ind() << "<ObjectData Count=\"" << obj.size() <<"\">" << endl;

    writer.incInd(); // indentation for 'Object name'
    for (it = obj.begin(); it != obj.end(); ++it) {
        writer.Stream() << writer.ind() << "<Object name=\"" << (*it)->getExportName() << "\"";
        if((*it)->hasExtensions())
            writer.Stream() << " Extensions=\"True\"";

        writer.Stream() << ">" << endl;
        (*it)->Save(writer);
        writer.Stream() << writer.ind() << "</Object>" << endl;
    }

    writer.decInd(); // indentation for 'Object name'
    writer.Stream() << writer.ind() << "</ObjectData>" << endl;
    writer.decInd();  // indentation for 'Objects count'
}

struct DepInfo {
    std::unordered_set<std::string> deps;
    int canLoadPartial = 0;
};

static void _loadDeps(const std::string &name, 
        std::unordered_map<std::string,bool> &objs, 
        const std::unordered_map<std::string,DepInfo> &deps) 
{
    auto it = deps.find(name);
    if(it == deps.end()) {
        objs.emplace(name,true);
        return;
    }
    if(it->second.canLoadPartial) {
        if(it->second.canLoadPartial == 1) {
            // canLoadPartial==1 means all its children will be created but not
            // restored, i.e. exists as if newly created object, and therefore no
            // need to load dependency of the children
            for(auto &dep : it->second.deps)
                objs.emplace(dep,false);
            objs.emplace(name,true);
        }else
            objs.emplace(name,false);
        return;
    }
    objs[name] = true;
    // If cannot load partial, then recurse to load all children dependency
    for(auto &dep : it->second.deps) {
        auto it = objs.find(dep);
        if(it!=objs.end() && it->second)
            continue;
        _loadDeps(dep,objs,deps);
    }
}

std::vector<App::DocumentObject*>
Document::readObjects(Base::XMLReader& reader)
{
    d->touchedObjs.clear();
    bool keepDigits = testStatus(Document::KeepTrailingDigits);
    setStatus(Document::KeepTrailingDigits, !reader.doNameMapping());
    std::vector<App::DocumentObject*> objs;


    // read the object types
    reader.readElement("Objects");
    int Cnt = reader.getAttributeAsInteger("Count");

    if(!reader.hasAttribute(FC_ATTR_DEPENDENCIES))
        d->partialLoadObjects.clear();
    else if(d->partialLoadObjects.size()) {
        std::unordered_map<std::string,DepInfo> deps;
        for (int i=0 ;i<Cnt ;i++) {
            reader.readElement(FC_ELEMENT_OBJECT_DEPS);
            int dcount = reader.getAttributeAsInteger(FC_ATTR_DEP_COUNT);
            if(!dcount)
                continue;
            auto &info = deps[reader.getAttribute(FC_ATTR_DEP_OBJ_NAME)];
            if(reader.hasAttribute(FC_ATTR_DEP_ALLOW_PARTIAL))
                info.canLoadPartial = reader.getAttributeAsInteger(FC_ATTR_DEP_ALLOW_PARTIAL);
            for(int j=0;j<dcount;++j) {
                reader.readElement(FC_ELEMENT_OBJECT_DEP);
                const char *name = reader.getAttribute(FC_ATTR_DEP_OBJ_NAME);
                if(name && name[0])
                    info.deps.insert(name);
            }
            reader.readEndElement(FC_ELEMENT_OBJECT_DEPS);
        }
        std::vector<std::string> objs;
        objs.reserve(d->partialLoadObjects.size());
        for(auto &v : d->partialLoadObjects)
            objs.push_back(v.first.c_str());
        for(auto &name : objs)
            _loadDeps(name,d->partialLoadObjects,deps);
        if(Cnt > (int)d->partialLoadObjects.size())
            setStatus(Document::PartialDoc,true);
        else {
            for(auto &v : d->partialLoadObjects) {
                if(!v.second) {
                    setStatus(Document::PartialDoc,true);
                    break;
                }
            }
            if(!testStatus(Document::PartialDoc))
                d->partialLoadObjects.clear();
        }
    }

    long lastId = 0;
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Object");
        std::string type = reader.getAttribute("type");
        std::string name = reader.getAttribute("name");
        const char *viewType = reader.hasAttribute("ViewType")?reader.getAttribute("ViewType"):0;

        bool partial = false;
        if(d->partialLoadObjects.size()) {
            auto it = d->partialLoadObjects.find(name);
            if(it == d->partialLoadObjects.end())
                continue;
            partial = !it->second;
        }

        if(!testStatus(Status::Importing) && reader.hasAttribute("id")) {
            // if not importing, then temporary reset lastObjectId and make the
            // following addObject() generate the correct id for this object.
            d->lastObjectId = reader.getAttributeAsInteger("id")-1;
        }

        // To prevent duplicate name when export/import of objects from
        // external documents, we append those external object name with
        // @<document name>. Before importing (here means we are called by
        // importObjects), we shall strip the postfix. What the caller
        // (MergeDocument) sees is still the unstripped name mapped to a new
        // internal name, and the rest of the link properties will be able to
        // correctly unmap the names.
        auto pos = name.find('@');
        std::string _obj_name;
        const char *obj_name;
        if(pos!=std::string::npos) {
            _obj_name = name.substr(0,pos);
            obj_name = _obj_name.c_str();
        }else
            obj_name = name.c_str();

        try {
            // Use name from XML as is and do NOT remove trailing digits because
            // otherwise we may cause a dependency to itself
            // Example: Object 'Cut001' references object 'Cut' and removing the
            // digits we make an object 'Cut' referencing itself.
            App::DocumentObject* obj = addObject(type.c_str(), obj_name, /*isNew=*/ false, viewType, partial);
            if (obj) {
                if(lastId < obj->_Id)
                    lastId = obj->_Id;
                objs.push_back(obj);
                // use this name for the later access because an object with
                // the given name may already exist
                reader.addName(name.c_str(), obj->getNameInDocument());

                // restore touch/error status flags
                if (reader.hasAttribute("Touched")) {
                    if(reader.getAttributeAsInteger("Touched") != 0)
                        d->touchedObjs.insert(obj);
                }
                if (reader.hasAttribute("Invalid")) {
                    obj->setStatus(ObjectStatus::Error, reader.getAttributeAsInteger("Invalid") != 0);
                    if(obj->isError() && reader.hasAttribute("Error")) 
                        d->addRecomputeLog(reader.getAttribute("Error"),obj);
                }
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Cannot create object '%s': (%s)\n", name.c_str(), e.what());
        }
    }
    if(!testStatus(Status::Importing))
        d->lastObjectId = lastId;

    reader.readEndElement("Objects");
    setStatus(Document::KeepTrailingDigits, keepDigits);

    // read the features itself
    reader.clearPartialRestoreDocumentObject();
    reader.readElement("ObjectData");
    Cnt = reader.getAttributeAsInteger("Count");
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Object");
        std::string name = reader.getName(reader.getAttribute("name"));
        DocumentObject* pObj = getObject(name.c_str());
        if (pObj && !pObj->testStatus(App::PartialObject)) { // check if this feature has been registered
            pObj->setStatus(ObjectStatus::Restore, true);
            try {
                FC_TRACE("restoring " << pObj->getFullName());
                pObj->Restore(reader);
            }
            // Try to continue only for certain exception types if not handled
            // by the feature type. For all other exception types abort the process.
            catch (const Base::UnicodeError &e) {
                e.ReportException();
            }
            catch (const Base::ValueError &e) {
                e.ReportException();
            }
            catch (const Base::IndexError &e) {
                e.ReportException();
            }
            catch (const Base::RuntimeError &e) {
                e.ReportException();
            }

            pObj->setStatus(ObjectStatus::Restore, false);

            if (reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInDocumentObject)) {
                Base::Console().Error("Object \"%s\" was subject to a partial restore. As a result geometry may have changed or be incomplete.\n",name.c_str());
                reader.clearPartialRestoreDocumentObject();
            }
        }
        reader.readEndElement("Object");
    }
    reader.readEndElement("ObjectData");

    return objs;
}

void Document::addRecomputeObject(DocumentObject *obj) {
    if(testStatus(Status::Restoring) && obj) {
        d->touchedObjs.insert(obj);
        obj->touch();
    }
}

std::vector<App::DocumentObject*>
Document::importObjects(Base::XMLReader& reader)
{
    d->hashers.clear();
    Base::FlagToggler<> flag(_IsRestoring,false);
    Base::ObjectStatusLocker<Status, Document> restoreBit(Status::Restoring, this);
    Base::ObjectStatusLocker<Status, Document> restoreBit2(Status::Importing, this);
    ExpressionParser::ExpressionImporter expImporter(reader);

    reader.readElement("Document");
    long scheme = reader.getAttributeAsInteger("SchemaVersion");
    reader.DocumentSchema = scheme;
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute("ProgramVersion");
    } else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = reader.getAttributeAsUnsigned("FileVersion");
    } else {
        reader.FileVersion = 0;
    }

    std::vector<App::DocumentObject*> objs = readObjects(reader);
    for(auto o : objs) {
        if(o && o->getNameInDocument()) {
            o->setStatus(App::ObjImporting,true);
            FC_LOG("importing " << o->getFullName());
        }
    }

    reader.readEndElement("Document");

    signalImportObjects(objs, reader);
    afterRestore(objs);

    signalFinishImportObjects(objs);

    for(auto o : objs) {
        if(o && o->getNameInDocument())
            o->setStatus(App::ObjImporting,false);
    }
    d->hashers.clear();
    return objs;
}

unsigned int Document::getMemSize (void) const
{
    unsigned int size = 0;

    // size of the DocObjects in the document
    std::vector<DocumentObject*>::const_iterator it;
    for (it = d->objectArray.begin(); it != d->objectArray.end(); ++it)
        size += (*it)->getMemSize();

    size += Hasher->getMemSize();

    // size of the document properties...
    size += PropertyContainer::getMemSize();

    // Undo Redo size
    size += getUndoMemSize();

    return size;
}

static std::string checkFileName(const char *file) {
    std::string fn(file);

    // Append extension if missing. This option is added for security reason, so
    // that the user won't accidently overwrite other file that may be critical.
    if(App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetBool("CheckExtension",true))
    {
        const char *ext = strrchr(file,'.');
        if(!ext || !boost::iequals(ext+1,"fcstd")) {
            if(ext && ext[1] == 0)
                fn += "fcstd";
            else
                fn += ".fcstd";
        }
    }
    return fn;
}

bool Document::saveAs(const char* _file)
{
    std::string file = checkFileName(_file);
    Base::FileInfo fi(file.c_str());
    if (this->FileName.getStrValue() != file) {
        this->FileName.setValue(file);
        this->Label.setValue(fi.fileNamePure());
        this->Uid.touch(); // this forces a rename of the transient directory
    }

    return save();
}

bool Document::saveCopy(const char* _file) const
{
    std::string file = checkFileName(_file);
    if (this->FileName.getStrValue() != file) {
        bool result = saveToFile(file.c_str());
        return result;
    }
    return false;
}

// Save the document under the name it has been opened
bool Document::save (void)
{
    if(testStatus(Document::PartialDoc)) {
        FC_ERR("Partial loaded document '" << Label.getValue() << "' cannot be saved");
        // TODO We don't make this a fatal error and return 'true' to make it possible to
        // save other documents that depends on this partial opened document. We need better
        // handling to avoid touching partial documents.
        return true;
    }

    if (*(FileName.getValue()) != '\0') {
        // Save the name of the tip object in order to handle in Restore()
        if (Tip.getValue()) {
            TipName.setValue(Tip.getValue()->getNameInDocument());
        }

        std::string LastModifiedDateString = Base::TimeInfo::currentDateTimeString();
        LastModifiedDate.setValue(LastModifiedDateString.c_str());
        // set author if needed
        bool saveAuthor = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document")->GetBool("prefSetAuthorOnSave",false);
        if (saveAuthor) {
            std::string Author = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetASCII("prefAuthor","");
            LastModifiedBy.setValue(Author.c_str());
        }

        return saveToFile(FileName.getValue());
    }

    return false;
}

bool Document::saveToFile(const char* filename) const
{
    signalStartSave(*this, filename);

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    int compression = hGrp->GetInt("CompressionLevel",3);
    compression = Base::clamp<int>(compression, Z_NO_COMPRESSION, Z_BEST_COMPRESSION);

    bool policy = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetBool("BackupPolicy",true);

    // make a tmp. file where to save the project data first and then rename to
    // the actual file name. This may be useful if overwriting an existing file
    // fails so that the data of the work up to now isn't lost.
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = filename;
    if (policy) {
        fn += ".";
        fn += uuid;
    }
    Base::FileInfo tmp(fn);

    // open extra scope to close ZipWriter properly
    {
        Base::ofstream file(tmp, std::ios::out | std::ios::binary);
        Base::ZipWriter writer(file);
        if (!file.is_open()) {
            throw Base::FileException("Failed to open file", tmp);
        }

        writer.setComment("FreeCAD Document");
        writer.setLevel(compression);
        writer.putNextEntry("Document.xml");

        if (hGrp->GetBool("SaveBinaryBrep", false))
            writer.setMode("BinaryBrep");

        writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl
                        << "<!--" << endl
                        << " FreeCAD Document, see http://www.freecadweb.org for more information..." << endl
                        << "-->" << endl;
        Document::Save(writer);

        // Special handling for Gui document.
        signalSaveDocument(writer);

        // write additional files
        writer.writeFiles();

        if (writer.hasErrors()) {
            throw Base::FileException("Failed to write all data to file", tmp);
        }

        GetApplication().signalSaveDocument(*this);
    }

    if (policy) {
        // if saving the project data succeeded rename to the actual file name
        Base::FileInfo fi(filename);
        if (fi.exists()) {
            bool backup = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetBool("CreateBackupFiles",true);
            int count_bak = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetInt("CountBackupFiles",1);
            if (backup) {
                int nSuff = 0;
                std::string fn = fi.fileName();
                Base::FileInfo di(fi.dirPath());
                std::vector<Base::FileInfo> backup;
                std::vector<Base::FileInfo> files = di.getDirectoryContent();
                for (std::vector<Base::FileInfo>::iterator it = files.begin(); it != files.end(); ++it) {
                    std::string file = it->fileName();
                    if (file.substr(0,fn.length()) == fn) {
                        // starts with the same file name
                        std::string suf(file.substr(fn.length()));
                        if (suf.size() > 0) {
                            std::string::size_type nPos = suf.find_first_not_of("0123456789");
                            if (nPos==std::string::npos) {
                                // store all backup files
                                backup.push_back(*it);
                                nSuff = std::max<int>(nSuff, std::atol(suf.c_str()));
                            }
                        }
                    }
                }

                if (!backup.empty() && (int)backup.size() >= count_bak) {
                    // delete the oldest backup file we found
                    Base::FileInfo del = backup.front();
                    for (std::vector<Base::FileInfo>::iterator it = backup.begin(); it != backup.end(); ++it) {
                        if (it->lastModified() < del.lastModified())
                            del = *it;
                    }

                    del.deleteFile();
                    fn = del.filePath();
                }
                else {
                    // create a new backup file
                    std::stringstream str;
                    str << fi.filePath() << (nSuff + 1);
                    fn = str.str();
                }

                if (fi.renameFile(fn.c_str()) == false)
                    Base::Console().Warning("Cannot rename project file to backup file\n");
            }
            else {
                fi.deleteFile();
            }
        }
        if (tmp.renameFile(filename) == false) {
            Base::Console().Warning("Cannot rename file from '%s' to '%s'\n",
                                    fn.c_str(), filename);
        }
    }

    signalFinishSave(*this, filename);

    return true;
}

bool Document::isAnyRestoring() {
    return _IsRestoring;
}

// Open the document
void Document::restore (const char *filename,
        bool delaySignal, const std::set<std::string> &objNames)
{
    clearUndos();
    d->activeObject = 0;

    if(d->objectArray.size()) {
        GetApplication().signalDeleteDocument(*this);
        d->objectArray.clear();
        for(auto &v : d->objectMap) {
            v.second->setStatus(ObjectStatus::Destroy, true);
            delete(v.second);
        }
        d->objectMap.clear();
        d->objectIdMap.clear();
        GetApplication().signalNewDocument(*this,false);
    }

    Base::FlagToggler<> flag(_IsRestoring,false);

    setStatus(Document::PartialDoc,false);

    d->clearRecomputeLog();
    d->objectArray.clear();
    d->objectMap.clear();
    d->objectIdMap.clear();
    d->lastObjectId = 0;

    if(!filename)
        filename = FileName.getValue();
    Base::FileInfo fi(filename);
    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    std::streambuf* buf = file.rdbuf();
    std::streamoff size = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    if (size < 22) // an empty zip archive has 22 bytes
        throw Base::FileException("Invalid project file",filename);

    zipios::ZipInputStream zipstream(file);
    Base::XMLReader reader(filename, zipstream);

    if (!reader.isValid())
        throw Base::FileException("Error reading compression file",filename);

    GetApplication().signalStartRestoreDocument(*this);
    setStatus(Document::Restoring, true);

    d->partialLoadObjects.clear();
    for(auto &name : objNames)
        d->partialLoadObjects.emplace(name,true);
    try {
        Document::Restore(reader);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Invalid Document.xml: %s\n", e.what());
    }
    d->partialLoadObjects.clear();

    // Special handling for Gui document, the view representations must already
    // exist, what is done in Restore().
    // Note: This file doesn't need to be available if the document has been created
    // without GUI. But if available then follow after all data files of the App document.
    signalRestoreDocument(reader);
    reader.readFiles(zipstream);

    if (reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestore)) {
        setStatus(Document::PartialRestore, true);
        Base::Console().Error("There were errors while loading the file. Some data might have been modified or not recovered at all. Look above for more specific information about the objects involved.\n");
    }

    if(!delaySignal)
        afterRestore(false,true);
}

void Document::afterRestore(bool checkXLink, bool checkPartial) {
    Base::FlagToggler<> flag(_IsRestoring,false);
    if(!afterRestore(d->objectArray,checkXLink,checkPartial)) {
        FC_WARN("Reload partial document " << getName());
        restore();
        return;
    }
    GetApplication().signalFinishRestoreDocument(*this);
    setStatus(Document::Restoring, false);
}

bool Document::afterRestore(const std::vector<DocumentObject *> &objArray, 
        bool checkXLink, bool checkPartial) 
{
    checkPartial = checkPartial && testStatus(Document::PartialDoc);
    if(checkPartial && d->touchedObjs.size())
        return false;

    // some link type property cannot restore link information until other
    // objects has been restored. For example, PropertyExpressionEngine and
    // PropertySheet with expression containing label reference. So we add the
    // Property::afterRestore() interface to let them sort it out. Note, this
    // API is not called in object dedpenency order, because the order
    // information is not ready yet.
    std::map<DocumentObject*, std::vector<App::Property*> > propMap;
    for(auto obj : objArray) {
        auto &props = propMap[obj];
        obj->getPropertyList(props);
        for(auto prop : props) {
            try {
                prop->afterRestore();
            } catch (const Base::Exception& e) {
                FC_ERR("Failed to restore " << obj->getFullName() 
                        << '.' << getPropertyName(prop) << ": " << e.what());
            }
        }
    }

    if(checkPartial && d->touchedObjs.size()) {
        // partial document touched, signal full reload
        return false;
    }

    std::set<DocumentObject*> objSet(objArray.begin(),objArray.end());
    auto objs = getDependencyList(objArray.empty()?d->objectArray:objArray,DepSort);
    for (auto obj : objs) {
        if(objSet.find(obj)==objSet.end())
            continue;
        try {
            for(auto prop : propMap[obj])
                prop->onContainerRestored();
            bool touched = false;
            auto returnCode = obj->ExpressionEngine.execute(
                    PropertyExpressionEngine::ExecuteOnRestore,&touched);
            if(returnCode!=DocumentObject::StdReturn) {
                FC_ERR("Expression engine failed to restore " << obj->getFullName() << ": " << returnCode->Why);
                d->addRecomputeLog(returnCode);
            }
            obj->onDocumentRestored();
            if(touched)
                d->touchedObjs.insert(obj);
        }
        catch (const Base::Exception& e) {
            d->addRecomputeLog(e.what(),obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << e.what());
        }
        catch (std::exception &e) {
            d->addRecomputeLog(e.what(),obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << e.what());
        }
        catch (...) {
            d->addRecomputeLog("Unknown exception on restore",obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << "unknown exception");
        }
        if(checkXLink && !d->touchedObjs.count(obj)) {
            auto &props = propMap[obj];
            props.clear();
            // refresh properties in case the object changes its property list
            obj->getPropertyList(props);
            for(auto prop : props) {
                auto link = Base::freecad_dynamic_cast<PropertyLinkBase>(prop);
                int res;
                if(link && (res=link->checkRestore())) {
                    d->touchedObjs.insert(obj);
                    if(res==1)
                        FC_WARN("'" << obj->getFullName() << "' xlink property '" 
                                << prop->getName() << "' time stamp changed");
                    else {
                        std::ostringstream ss;
                        ss << prop->getName() << " not restored";
                        d->addRecomputeLog(ss.str(),obj);
                        FC_WARN("'" << obj->getFullName() << "' xlink property '" 
                                << prop->getName() << "' not restored");
                    }
                    break;
                }
            }
        }

        if(checkPartial && d->touchedObjs.size()) {
            // partial document touched, signal full reload
            return false;
        } else if(!d->touchedObjs.count(obj)) 
            obj->purgeTouched();

        signalFinishRestoreObject(*obj);
    }

    d->touchedObjs.clear();
    return true;
}

bool Document::isSaved() const
{
    std::string name = FileName.getValue();
    return !name.empty();
}

/** Label is the visible name of a document shown e.g. in the windows title
 * or in the tree view. The label almost (but not always e.g. if you manually change it)
 * matches with the file name where the document is stored to.
 * In contrast to Label the method getName() returns the internal name of the document that only
 * matches with Label when loading or creating a document because then both are set to the same value.
 * Since the internal name cannot be changed during runtime it must differ from the Label after saving
 * the document the first time or saving it under a new file name.
 * @ note More than one document can have the same label name.
 * @ note The internal is always guaranteed to be unique because @ref Application::newDocument() checks
 * for a document with the same name and makes it unique if needed. Hence you cannot rely on that the
 * internal name matches with the name you passed to Application::newDoument(). You should use the
 * method getName() instead.
 */
const char* Document::getName() const
{
    return GetApplication().getDocumentName(this);
}

/// Remove all modifications. After this call The document becomes valid again.
void Document::purgeTouched()
{
    for (std::vector<DocumentObject*>::iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        (*It)->purgeTouched();
}

bool Document::isTouched() const
{
    for (std::vector<DocumentObject*>::const_iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        if ((*It)->isTouched())
            return true;
    return false;
}

vector<DocumentObject*> Document::getTouched(void) const
{
    vector<DocumentObject*> result;

    for (std::vector<DocumentObject*>::const_iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        if ((*It)->isTouched())
            result.push_back(*It);

    return result;
}

void Document::setClosable(bool c)
{
    setStatus(Document::Closable, c);
}

bool Document::isClosable() const
{
    return testStatus(Document::Closable);
}

int Document::countObjects(void) const
{
   return static_cast<int>(d->objectArray.size());
}

void Document::getLinksTo(std::set<DocumentObject*> &links, 
        const DocumentObject *obj, int options, int maxCount) const 
{
    std::map<const App::DocumentObject*,App::DocumentObject*> linkMap;

    for(auto o : d->objectArray) {
        if(o == obj) continue;
        auto linked = o;
        if(options & GetLinkArray) {
            auto ext = o->getExtensionByType<LinkBaseExtension>(true);
            if(ext) 
                linked = ext->getTrueLinkedObject(false,0,0,true);
            else
                linked = o->getLinkedObject(false);
        } else 
            linked = o->getLinkedObject(false);

        if(linked && linked!=o) {
            if(options & GetLinkRecursive)
                linkMap.emplace(linked,o);
            else if(linked == obj) {
                links.insert(o);
                if(maxCount && maxCount<=(int)links.size())
                    return;
            }
        }
    }

    if(!(options & GetLinkRecursive))
        return;

    std::vector<const DocumentObject*> current(1,obj);
    for(int depth=0;current.size();++depth) {
        if(!GetApplication().checkLinkDepth(depth))
            break;
        std::vector<const DocumentObject*> next;
        for(auto o : current) {
            auto iter = linkMap.find(o);
            if(iter!=linkMap.end() && links.insert(iter->second).second) {
                if(maxCount && maxCount<=(int)links.size())
                    return;
                next.push_back(iter->second);
            }
        }
        current.swap(next);
    }
    return;
}

bool Document::hasLinksTo(const DocumentObject *obj) const {
    std::set<DocumentObject *> links;
    getLinksTo(links,obj,App::GetLinkArray,1);
    return !links.empty();
}

std::vector<App::DocumentObject*> Document::getInList(const DocumentObject* me) const
{
    // result list
    std::vector<App::DocumentObject*> result;
    // go through all objects
    for (auto It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        // get the outList and search if me is in that list
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2)
            if (*It2 && *It2 == me)
                // add the parent object
                result.push_back(It->second);
    }
    return result;
}

// This function unifies the old _rebuildDependencyList() and
// getDependencyList().  The algorithm basically obtains the object dependency
// by recrusivly visiting the OutList of each object in the given object array.
// It makes sure to call getOutList() of each object once and only once, which
// makes it much more efficient than calling getRecursiveOutList() on each
// individual object.
//
// The problem with the original algorithm is that, it assumes the objects
// inside any OutList are all within the given object array, so it does not
// recursively call getOutList() on those dependent objects inside. This
// assumption is broken by the introduction of PropertyXLink which can link to
// external object.
//
static void _buildDependencyList(const std::vector<App::DocumentObject*> &objectArray,
        int options, std::vector<App::DocumentObject*> *depObjs, 
        DependencyList *depList, std::map<DocumentObject*,Vertex> *objectMap, 
        bool *touchCheck = 0)
{
    std::map<DocumentObject*, std::vector<DocumentObject*> > outLists;
    std::deque<DocumentObject*> objs;

    if(objectMap) objectMap->clear();
    if(depList) depList->clear();

    int op = (options & Document::DepNoXLinked)?DocumentObject::OutListNoXLinked:0;
    for (auto obj : objectArray) {
        objs.push_back(obj);
        while(objs.size()) {
            auto obj = objs.front();
            objs.pop_front();
            if(!obj || !obj->getNameInDocument())
                continue;

            auto it = outLists.find(obj);
            if(it!=outLists.end())
                continue;

            if(touchCheck) {
                if(obj->isTouched() || obj->mustExecute()) {
                    // early termination on touch check
                    *touchCheck = true;
                    return;
                }
            }
            if(depObjs) depObjs->push_back(obj);
            if(objectMap && depList)
                (*objectMap)[obj] = add_vertex(*depList);

            auto &outList = outLists[obj];
            outList = obj->getOutList(op);
            objs.insert(objs.end(),outList.begin(),outList.end());
        }
    }

    if(objectMap && depList) {
        for (const auto &v : outLists) {
            for(auto obj : v.second) {
                if(obj && obj->getNameInDocument())
                    add_edge((*objectMap)[v.first],(*objectMap)[obj],*depList);
            }
        }
    }
}

std::vector<App::DocumentObject*> Document::getDependencyList(
    const std::vector<App::DocumentObject*>& objectArray, int options)
{
    std::vector<App::DocumentObject*> ret;
    if(!(options & DepSort)) {
        _buildDependencyList(objectArray,options,&ret,0,0);
        return ret;
    }

    DependencyList depList;
    std::map<DocumentObject*,Vertex> objectMap;
    std::map<Vertex,DocumentObject*> vertexMap;

    _buildDependencyList(objectArray,options,0,&depList,&objectMap);

    for(auto &v : objectMap)
        vertexMap[v.second] = v.first;

    std::list<Vertex> make_order;
    try {
        boost::topological_sort(depList, std::front_inserter(make_order));
    } catch (const std::exception& e) {
        if(options & DepNoCycle) {
            // Use boost::strong_components to find cycles. It groups strongly
            // connected vertices as components, and therefore each component
            // forms a cycle.
            std::vector<int> c(vertexMap.size());
            std::map<int,std::vector<Vertex> > components;
            boost::strong_components(depList,boost::make_iterator_property_map(
                        c.begin(),boost::get(boost::vertex_index,depList),c[0]));
            for(size_t i=0;i<c.size();++i) 
                components[c[i]].push_back(i);

            FC_ERR("Dependency cycles: ");
            std::ostringstream ss;
            ss << std::endl;
            for(auto &v : components) {
                if(v.second.size()==1) {
                    // For components with only one member, we still need to
                    // check if there it is self looping.
                    auto it = vertexMap.find(v.second[0]);
                    if(it==vertexMap.end())
                        continue;
                    // Try search the object in its own out list
                    for(auto obj : it->second->getOutList()) {
                        if(obj == it->second) {
                            ss << std::endl << it->second->getFullName() << std::endl;
                            break;
                        }
                    }
                    continue;
                }
                // For components with more than one member, they form a loop together
                for(size_t i=0;i<v.second.size();++i) {
                    auto it = vertexMap.find(v.second[i]);
                    if(it==vertexMap.end())
                        continue;
                    if(i%6==0)
                        ss << std::endl;
                    ss << it->second->getFullName() << ", ";
                }
                ss << std::endl;
            }
            FC_ERR(ss.str());
            FC_THROWM(Base::RuntimeError, e.what());
        }
        FC_ERR(e.what());
        ret = DocumentP::partialTopologicalSort(objectArray);
        std::reverse(ret.begin(),ret.end());
        return ret;
    }

    for (std::list<Vertex>::reverse_iterator i = make_order.rbegin();i != make_order.rend(); ++i)
        ret.push_back(vertexMap[*i]);
    return ret;
}

std::vector<App::Document*> Document::getDependentDocuments(bool sort) {
    return getDependentDocuments({this},sort);
}

std::vector<App::Document*> Document::getDependentDocuments(
        std::vector<App::Document*> pending, bool sort) 
{
    DependencyList depList;
    std::map<Document*,Vertex> docMap;
    std::map<Vertex,Document*> vertexMap;

    std::vector<App::Document*> ret;
    if(pending.empty())
        return ret;

    auto outLists = PropertyXLink::getDocumentOutList();
    std::set<App::Document*> docs;
    docs.insert(pending.begin(),pending.end());
    if(sort) {
        for(auto doc : pending)
            docMap[doc] = add_vertex(depList);
    }
    while(pending.size()) {
        auto doc = pending.back();
        pending.pop_back();

        auto it = outLists.find(doc);
        if(it == outLists.end())
            continue;

        auto &vertex = docMap[doc];
        for(auto depDoc : it->second) {
            if(docs.insert(depDoc).second) {
                pending.push_back(depDoc);
                if(sort)
                    docMap[depDoc] = add_vertex(depList);
            }
            add_edge(vertex,docMap[depDoc],depList);
        }
    }

    if(!sort) {
        ret.insert(ret.end(),docs.begin(),docs.end());
        return ret;
    }

    std::list<Vertex> make_order;
    try {
        boost::topological_sort(depList, std::front_inserter(make_order));
    } catch (const std::exception& e) {
        std::string msg("Document::getDependentDocuments: ");
        msg += e.what();
        throw Base::RuntimeError(msg);
    }

    for(auto &v : docMap)
        vertexMap[v.second] = v.first;
    for (auto rIt=make_order.rbegin(); rIt!=make_order.rend(); ++rIt)
        ret.push_back(vertexMap[*rIt]);
    return ret;
}

void Document::_rebuildDependencyList(const std::vector<App::DocumentObject*> &objs)
{
#ifdef USE_OLD_DAG
    _buildDependencyList(objs.empty()?d->objectArray:objs,false,0,&d->DepList,&d->VertexObjectList);
#else
    (void)objs;
#endif
}

/**
 * @brief Signal that object identifiers, typically a property or document object has been renamed.
 *
 * This function iterates through all document object in the document, and calls its
 * renameObjectIdentifiers functions.
 *
 * @param paths Map with current and new names
 */

void Document::renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths, const std::function<bool(const App::DocumentObject*)> & selector)
{
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> extendedPaths;

    std::map<App::ObjectIdentifier, App::ObjectIdentifier>::const_iterator it = paths.begin();
    while (it != paths.end()) {
        extendedPaths[it->first.canonicalPath()] = it->second.canonicalPath();
        ++it;
    }

    for (std::vector<DocumentObject*>::iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it)
        if (selector(*it))
            (*it)->renameObjectIdentifiers(extendedPaths);
}

#ifdef USE_OLD_DAG
int Document::recompute(const std::vector<App::DocumentObject*> &objs, bool force)
{
    if (testStatus(Document::Recomputing)) {
        // this is clearly a bug in the calling instance
        throw Base::RuntimeError("Nested recomputes of a document are not allowed");
    }

    int objectCount = 0;

    // The 'SkipRecompute' flag can be (tmp.) set to avoid too many
    // time expensive recomputes
    if(!force && testStatus(Document::SkipRecompute))
        return 0;

    Base::ObjectStatusLocker<Document::Status, Document> exe(Document::Recomputing, this);

    // delete recompute log
    d->clearRecomputeLog();

    // updates the dependency graph
    _rebuildDependencyList(objs);

    std::list<Vertex> make_order;
    DependencyList::out_edge_iterator j, jend;

    try {
        // this sort gives the execute
        boost::topological_sort(d->DepList, std::front_inserter(make_order));
    }
    catch (const std::exception& e) {
        std::cerr << "Document::recompute: " << e.what() << std::endl;
        return -1;
    }

    // caching vertex to DocObject
    for (std::map<DocumentObject*,Vertex>::const_iterator It1= d->VertexObjectList.begin();It1 != d->VertexObjectList.end(); ++It1)
        d->vertexMap[It1->second] = It1->first;

#ifdef FC_LOGFEATUREUPDATE
    std::clog << "make ordering: " << std::endl;
#endif

    std::set<DocumentObject*> recomputeList;

    for (std::list<Vertex>::reverse_iterator i = make_order.rbegin();i != make_order.rend(); ++i) {
        DocumentObject* Cur = d->vertexMap[*i];
        // Because of PropertyXLink, we should account for external objects
        // TODO: make sure it is safe to rely on getNameInDocument() to check if
        // object is in the document. If it crashes, then we should fix the code
        // to properly nullify getNameInDocument(), rather than revert back to
        // the inefficient isIn()
        // if (!Cur || !isIn(Cur)) continue;
        if (!Cur || !Cur->getNameInDocument()) continue;
#ifdef FC_LOGFEATUREUPDATE
        std::clog << Cur->getNameInDocument() << " dep on:" ;
#endif
        bool NeedUpdate = false;

        // ask the object if it should be recomputed
        if (Cur->mustExecute() == 1 || Cur->ExpressionEngine.depsAreTouched()) {
#ifdef FC_LOGFEATUREUPDATE
            std::clog << "[touched]";
#endif
            NeedUpdate = true;
        }
        else {// if (Cur->mustExecute() == -1)
            // update if one of the dependencies is touched
            for (boost::tie(j, jend) = out_edges(*i, d->DepList); j != jend; ++j) {
                DocumentObject* Test = d->vertexMap[target(*j, d->DepList)];

                if (!Test) continue;
#ifdef FC_LOGFEATUREUPDATE
                std::clog << " " << Test->getNameInDocument();
#endif
                if (Test->isTouched()) {
                    NeedUpdate = true;
#ifdef FC_LOGFEATUREUPDATE
                    std::clog << "[touched]";
#endif
                }
            }
        }
        // if one touched recompute
        if (NeedUpdate) {
            Cur->touch();
#ifdef FC_LOGFEATUREUPDATE
            std::clog << " => Recompute feature";
#endif
            recomputeList.insert(Cur);
        }
#ifdef FC_LOGFEATUREUPDATE
        std::clog << std::endl;
#endif
    }

#ifdef FC_LOGFEATUREUPDATE
    std::clog << "Have to recompute the following document objects" << std::endl;
    for (std::set<DocumentObject*>::const_iterator it = recomputeList.begin(); it != recomputeList.end(); ++it) {
        std::clog << "  " << (*it)->getNameInDocument() << std::endl;
    }
#endif

    for (std::list<Vertex>::reverse_iterator i = make_order.rbegin();i != make_order.rend(); ++i) {
        DocumentObject* Cur = d->vertexMap[*i];
        if (!Cur || !isIn(Cur)) continue;

        if (recomputeList.find(Cur) != recomputeList.end() ||
                Cur->ExpressionEngine.depsAreTouched()) {
            if ( _recomputeFeature(Cur)) {
                // if something happened break execution of recompute
                d->vertexMap.clear();
                return -1;
            }
            signalRecomputedObject(*Cur);
            ++objectCount;
        }
    }

    // reset all touched
    for (std::map<Vertex,DocumentObject*>::iterator it = d->vertexMap.begin(); it != d->vertexMap.end(); ++it) {
        // TODO: check the TODO comments above for details
        // if ((it->second) && isIn(it->second))
        if ((it->second) && it->second->getNameInDocument())
            it->second->purgeTouched();
    }
    d->vertexMap.clear();

    signalRecomputed(*this);

    return objectCount;
}

#else //ifdef USE_OLD_DAG

int Document::recompute(const std::vector<App::DocumentObject*> &objs, bool force, bool *hasError, int options) 
{
    ExpressionParser::clearWarning();

    int objectCount = 0;

    if (testStatus(Document::PartialDoc)) {
        FC_ERR("Cannot recompute partially loaded document: " << getName());
        return 0;
    }

    if (testStatus(Document::Recomputing)) {
        // this is clearly a bug in the calling instance
        throw Base::RuntimeError("Nested recomputes of a document are not allowed");
    }
    // The 'SkipRecompute' flag can be (tmp.) set to avoid too many
    // time expensive recomputes
    if(!force && testStatus(Document::SkipRecompute)) {
        signalSkipRecompute(*this,objs);
        return 0;
    }

    // delete recompute log
    d->clearRecomputeLog();

    //do we have anything to do?
    if(d->objectMap.empty())
        return 0;

    Base::ObjectStatusLocker<Document::Status, Document> exe(Document::Recomputing, this);
    signalBeforeRecompute(*this);

#if 0
    //////////////////////////////////////////////////////////////////////////
    // Comment by Realthunder: 
    // the topologicalSrot() below cannot handle partial recompute, haven't got
    // time to figure out the code yet, simply use back boost::topological_sort
    // for now, that is, rely on getDependencyList() to do the sorting. The
    // downside is, it didn't take advantage of the ready built InList, nor will
    // it report for cyclic dependency.
    //////////////////////////////////////////////////////////////////////////

    // get the sorted vector of all dependent objects and go though it from the end
    auto depObjs = getDependencyList(objs.empty()?d->objectArray:objs);
    vector<DocumentObject*> topoSortedObjects = topologicalSort(depObjs);
    if (topoSortedObjects.size() != depObjs.size()){
        cerr << "App::Document::recompute(): cyclic dependency detected" << endl;
        topoSortedObjects = d->partialTopologicalSort(depObjs);
    }
    std::reverse(topoSortedObjects.begin(),topoSortedObjects.end());
#else
    auto topoSortedObjects = getDependencyList(objs.empty()?d->objectArray:objs,DepSort|options);
#endif
    for(auto obj : topoSortedObjects)
        obj->setStatus(ObjectStatus::PendingRecompute,true);

    std::set<App::DocumentObject *> filter;
    size_t idx = 0;
    try {
        // maximum two passes to allow some form of dependency inversion
        for(int passes=0; passes<2 && idx<topoSortedObjects.size(); ++passes) {
            Base::SequencerLauncher seq("Recompute...", topoSortedObjects.size());
            FC_LOG("Recompute pass " << passes);
            for (;idx<topoSortedObjects.size();seq.next(true),++idx) {
                auto obj = topoSortedObjects[idx];
                if(!obj->getNameInDocument() || filter.find(obj)!=filter.end())
                    continue;
                // ask the object if it should be recomputed
                bool doRecompute = false;
                if (obj->mustRecompute()) {
                    doRecompute = true;
                    ++objectCount;
                    int res = _recomputeFeature(obj);
                    if(res) {
                        if(hasError)
                            *hasError = true;
                        if(res < 0) {
                            passes = 2;
                            break;
                        }
                        // if something happened filter all object in its
                        // inListRecursive from the queue then proceed
                        obj->getInListEx(filter,true);
                        filter.insert(obj);
                        continue;
                    }
                }
                if(obj->isTouched() || doRecompute) {
                    signalRecomputedObject(*obj);
                    obj->purgeTouched();
                    // set all dependent object touched to force recompute
                    for (auto inObjIt : obj->getInList())
                        inObjIt->enforceRecompute();
                }
            }
            // check if all objects are recomputed but still thouched 
            for (size_t i=0;i<topoSortedObjects.size();++i) {
                auto obj = topoSortedObjects[i];
                obj->setStatus(ObjectStatus::Recompute2,false);
                if(!filter.count(obj) && obj->isTouched()) {
                    if(passes>0) 
                        FC_ERR(obj->getFullName() << " still touched after recompute");
                    else{
                        FC_LOG(obj->getFullName() << " still touched after recompute");
                        if(idx>=topoSortedObjects.size()) {
                            // let's start the next pass on the first touched object
                            idx = i;
                        }
                        obj->setStatus(ObjectStatus::Recompute2,true);
                    }
                }
            }
        }
    }catch(Base::Exception &e) {
        e.ReportException();
    }

    for(auto obj : topoSortedObjects) {
        if(!obj->getNameInDocument())
            continue;
        obj->setStatus(ObjectStatus::PendingRecompute,false);
        obj->setStatus(ObjectStatus::Recompute2,false);
        if(obj->testStatus(ObjectStatus::PendingRemove))
            obj->getDocument()->removeObject(obj->getNameInDocument());
    }

    signalRecomputed(*this,topoSortedObjects);

    return objectCount;
}

#endif // USE_OLD_DAG

/*!
  Does almost the same as topologicalSort() until no object with an input degree of zero
  can be found. It then searches for objects with an output degree of zero until neither
  an object with input or output degree can be found. The remaining objects form one or
  multiple cycles.
  An alternative to this method might be:
  https://en.wikipedia.org/wiki/Tarjan%E2%80%99s_strongly_connected_components_algorithm
 */
std::vector<App::DocumentObject*> DocumentP::partialTopologicalSort(
        const std::vector<App::DocumentObject*>& objects)
{
    vector < App::DocumentObject* > ret;
    ret.reserve(objects.size());
    // pairs of input and output degree
    map < App::DocumentObject*, std::pair<int, int> > countMap;

    for (auto objectIt : objects) {
        //we need inlist with unique entries
        auto in = objectIt->getInList();
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());

        //we need outlist with unique entries
        auto out = objectIt->getOutList();
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());

        countMap[objectIt] = std::make_pair(in.size(), out.size());
    }

    std::list<App::DocumentObject*> degIn;
    std::list<App::DocumentObject*> degOut;

    bool removeVertex = true;
    while (removeVertex) {
        removeVertex = false;

        // try input degree
        auto degInIt = find_if(countMap.begin(), countMap.end(),
                               [](pair< App::DocumentObject*, pair<int, int> > vertex)->bool {
            return vertex.second.first == 0;
        });

        if (degInIt != countMap.end()) {
            removeVertex = true;
            degIn.push_back(degInIt->first);
            degInIt->second.first = degInIt->second.first - 1;

            //we need outlist with unique entries
            auto out = degInIt->first->getOutList();
            std::sort(out.begin(), out.end());
            out.erase(std::unique(out.begin(), out.end()), out.end());

            for (auto outListIt : out) {
                auto outListMapIt = countMap.find(outListIt);
                if (outListMapIt != countMap.end())
                    outListMapIt->second.first = outListMapIt->second.first - 1;
            }
        }
    }

    // make the output degree negative if input degree is negative
    // to mark the vertex as processed
    for (auto& countIt : countMap) {
        if (countIt.second.first < 0) {
            countIt.second.second = -1;
        }
    }

    removeVertex = degIn.size() != objects.size();
    while (removeVertex) {
        removeVertex = false;

        auto degOutIt = find_if(countMap.begin(), countMap.end(),
                               [](pair< App::DocumentObject*, pair<int, int> > vertex)->bool {
            return vertex.second.second == 0;
        });

        if (degOutIt != countMap.end()) {
            removeVertex = true;
            degOut.push_front(degOutIt->first);
            degOutIt->second.second = degOutIt->second.second - 1;

            //we need inlist with unique entries
            auto in = degOutIt->first->getInList();
            std::sort(in.begin(), in.end());
            in.erase(std::unique(in.begin(), in.end()), in.end());

            for (auto inListIt : in) {
                auto inListMapIt = countMap.find(inListIt);
                if (inListMapIt != countMap.end())
                    inListMapIt->second.second = inListMapIt->second.second - 1;
            }
        }
    }

    // at this point we have no root object any more
    for (auto countIt : countMap) {
        if (countIt.second.first > 0 && countIt.second.second > 0) {
            degIn.push_back(countIt.first);
        }
    }

    ret.insert(ret.end(), degIn.begin(), degIn.end());
    ret.insert(ret.end(), degOut.begin(), degOut.end());

    return ret;
}

std::vector<App::DocumentObject*> DocumentP::topologicalSort(const std::vector<App::DocumentObject*>& objects) const
{
    // topological sort algorithm described here:
    // https://de.wikipedia.org/wiki/Topologische_Sortierung#Algorithmus_f.C3.BCr_das_Topologische_Sortieren
    vector < App::DocumentObject* > ret;
    ret.reserve(objects.size());
    map < App::DocumentObject*,int > countMap;

    for (auto objectIt : objects) {
        // We now support externally linked objects
        // if(!obj->getNameInDocument() || obj->getDocument()!=this)
        if(!objectIt->getNameInDocument())
            continue;
        //we need inlist with unique entries
        auto in = objectIt->getInList();
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());

        countMap[objectIt] = in.size();
    }

    auto rootObjeIt = find_if(countMap.begin(), countMap.end(), [](pair < App::DocumentObject*, int > count)->bool {
        return count.second == 0;
    });

    if (rootObjeIt == countMap.end()){
        cerr << "Document::topologicalSort: cyclic dependency detected (no root object)" << endl;
        return ret;
    }

    while (rootObjeIt != countMap.end()){
        rootObjeIt->second = rootObjeIt->second - 1;

        //we need outlist with unique entries
        auto out = rootObjeIt->first->getOutList();
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());

        for (auto outListIt : out) {
            auto outListMapIt = countMap.find(outListIt);
            if (outListMapIt != countMap.end())
                outListMapIt->second = outListMapIt->second - 1;
        }
        ret.push_back(rootObjeIt->first);

        rootObjeIt = find_if(countMap.begin(), countMap.end(), [](pair < App::DocumentObject*, int > count)->bool {
            return count.second == 0;
        });
    }

    return ret;
}

std::vector<App::DocumentObject*> Document::topologicalSort() const
{
    return d->topologicalSort(d->objectArray);
}

const char * Document::getErrorDescription(const App::DocumentObject*Obj) const
{
    return d->findRecomputeLog(Obj);
}

// call the recompute of the Feature and handle the exceptions and errors.
int Document::_recomputeFeature(DocumentObject* Feat)
{
    FC_LOG("Recomputing " << Feat->getFullName());

    DocumentObjectExecReturn  *returnCode = 0;
    try {
        returnCode = Feat->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteNonOutput);
        if (returnCode == DocumentObject::StdReturn) {
            returnCode = Feat->recompute();
            if(returnCode == DocumentObject::StdReturn)
                returnCode = Feat->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteOutput);
        }
    }
    catch(Base::AbortException &e){
        e.ReportException();
        d->addRecomputeLog("User abort",Feat);
        return -1;
    }
    catch (const Base::MemoryException& e) {
        FC_ERR("Memory exception in " << Feat->getFullName() << " thrown: " << e.what());
        d->addRecomputeLog("Out of memory exception",Feat);
        return 1;
    }
    catch (Base::Exception &e) {
        e.ReportException();
        d->addRecomputeLog(e.what(),Feat);
        return 1;
    }
    catch (std::exception &e) {
        FC_ERR("exception in " << Feat->getFullName() << " thrown: " << e.what());
        d->addRecomputeLog(e.what(),Feat);
        return 1;
    }
#ifndef FC_DEBUG
    catch (...) {
        FC_ERR("Unknown exception in " << Feat->getFullName() << " thrown");
        d->addRecomputeLog("Unknown exception!",Feat);
        return 1;
    }
#endif

    if(returnCode == DocumentObject::StdReturn) {
        Feat->resetError();
    }else{
        returnCode->Which = Feat;
        d->addRecomputeLog(returnCode);
#ifdef FC_DEBUG
        FC_ERR("Failed to recompute " << Feat->getFullName() << ": " << returnCode->Why);
#else
        FC_LOG("Failed to recompute " << Feat->getFullName() << ": " << returnCode->Why);
#endif
        return 1;
    }
    return 0;
}

bool Document::recomputeFeature(DocumentObject* Feat, bool recursive)
{
    // delete recompute log
    d->clearRecomputeLog(Feat);

    // verify that the feature is (active) part of the document
    if (Feat->getNameInDocument()) {
        if(recursive) {
            bool hasError = false;
            recompute({Feat},true,&hasError);
            return !hasError;
        } else {
            _recomputeFeature(Feat);
            signalRecomputedObject(*Feat);
            return Feat->isValid();
        }
    }else
        return false;
}

DocumentObject * Document::addObject(const char* sType, const char* pObjectName, 
        bool isNew, const char *viewType, bool isPartial)
{
    Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(sType,true));

    string ObjectName;
    if (!base)
        return 0;
    if (!base->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        delete base;
        std::stringstream str;
        str << "'" << sType << "' is not a document object type";
        throw Base::TypeError(str.str());
    }

    App::DocumentObject* pcObject = static_cast<App::DocumentObject*>(base);
    pcObject->setDocument(this);

    // do no transactions if we do a rollback!
    if (!d->rollback) {
        // Undo stuff
        _checkTransaction(0,0,__LINE__);
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectDel(pcObject);
    }

    // get Unique name
    if (pObjectName && pObjectName[0] != '\0')
        ObjectName = getUniqueObjectName(pObjectName);
    else
        ObjectName = getUniqueObjectName(sType);


    d->activeObject = pcObject;

    // insert in the name map
    d->objectMap[ObjectName] = pcObject;
    // generate object id and add to id map;
    pcObject->_Id = ++d->lastObjectId;
    d->objectIdMap[pcObject->_Id] = pcObject;
    // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);
    // insert in the vector
    d->objectArray.push_back(pcObject);
    // insert in the adjacence list and reference through the ConectionMap
    //_DepConMap[pcObject] = add_vertex(_DepList);

    // If we are restoring, don't set the Label object now; it will be restored later. This is to avoid potential duplicate
    // label conflicts later.
    if (!d->StatusBits.test(Restoring))
        pcObject->Label.setValue( ObjectName );

    // Call the object-specific initialization
    if (!d->undoing && !d->rollback && isNew) {
        pcObject->setupObject ();
    }

    // mark the object as new (i.e. set status bit 2) and send the signal
    pcObject->setStatus(ObjectStatus::New, true);

    pcObject->setStatus(ObjectStatus::PartialObject, isPartial);

    if(!viewType)
        viewType = pcObject->getViewProviderNameOverride();
    if(viewType) 
        pcObject->_pcViewProviderName = viewType;

    signalNewObject(*pcObject);

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        signalTransactionAppend(*pcObject, d->activeUndoTransaction);
    }

    signalActivatedObject(*pcObject);

    // return the Object
    return pcObject;
}

std::vector<DocumentObject *> Document::addObjects(const char* sType, const std::vector<std::string>& objectNames, bool isNew)
{
    Base::Type::importModule(sType);
    Base::Type type = Base::Type::fromName(sType);
    if (!type.isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        std::stringstream str;
        str << "'" << sType << "' is not a document object type";
        throw Base::TypeError(str.str());
    }

    std::vector<DocumentObject *> objects;
    objects.resize(objectNames.size());
    std::generate(objects.begin(), objects.end(),
                  [&]{ return static_cast<App::DocumentObject*>(type.createInstance()); });

    // get all existing object names
    std::vector<std::string> reservedNames;
    reservedNames.reserve(d->objectMap.size());
    for (auto pos = d->objectMap.begin();pos != d->objectMap.end();++pos) {
        reservedNames.push_back(pos->first);
    }

    for (auto it = objects.begin(); it != objects.end(); ++it) {
        auto index = std::distance(objects.begin(), it);
        App::DocumentObject* pcObject = *it;
        pcObject->setDocument(this);

        // do no transactions if we do a rollback!
        if (!d->rollback) {
            // Undo stuff
            _checkTransaction(0,0,__LINE__);
            if (d->activeUndoTransaction) {
                d->activeUndoTransaction->addObjectDel(pcObject);
            }
        }

        // get unique name
        std::string ObjectName = objectNames[index];
        if (ObjectName.empty())
            ObjectName = sType;
        ObjectName = Base::Tools::getIdentifier(ObjectName);
        if (d->objectMap.find(ObjectName) != d->objectMap.end()) {
            // remove also trailing digits from clean name which is to avoid to create lengthy names
            // like 'Box001001'
            if (!testStatus(KeepTrailingDigits)) {
                std::string::size_type index = ObjectName.find_last_not_of("0123456789");
                if (index+1 < ObjectName.size()) {
                    ObjectName = ObjectName.substr(0,index+1);
                }
            }

            ObjectName = Base::Tools::getUniqueName(ObjectName, reservedNames, 3);
        }

        reservedNames.push_back(ObjectName);

        // insert in the name map
        d->objectMap[ObjectName] = pcObject;
        // generate object id and add to id map;
        pcObject->_Id = ++d->lastObjectId;
        d->objectIdMap[pcObject->_Id] = pcObject;
        // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
        pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);
        // insert in the vector
        d->objectArray.push_back(pcObject);

        pcObject->Label.setValue(ObjectName);

        // Call the object-specific initialization
        if (!d->undoing && !d->rollback && isNew) {
            pcObject->setupObject();
        }

        // mark the object as new (i.e. set status bit 2) and send the signal
        pcObject->setStatus(ObjectStatus::New, true);

        const char *viewType = pcObject->getViewProviderNameOverride();
        pcObject->_pcViewProviderName = viewType?viewType:"";

        signalNewObject(*pcObject);

        // do no transactions if we do a rollback!
        if (!d->rollback && d->activeUndoTransaction) {
            signalTransactionAppend(*pcObject, d->activeUndoTransaction);
        }
    }

    if (!objects.empty()) {
        d->activeObject = objects.back();
        signalActivatedObject(*objects.back());
    }

    return objects;
}

void Document::addObject(DocumentObject* pcObject, const char* pObjectName)
{
    if (pcObject->getDocument()) {
        throw Base::RuntimeError("Document object is already added to a document");
    }

    pcObject->setDocument(this);

    // do no transactions if we do a rollback!
    if (!d->rollback) {
        // Undo stuff
        _checkTransaction(0,0,__LINE__);
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectDel(pcObject);
    }

    // get unique name
    string ObjectName;
    if (pObjectName && pObjectName[0] != '\0')
        ObjectName = getUniqueObjectName(pObjectName);
    else
        ObjectName = getUniqueObjectName(pcObject->getTypeId().getName());

    d->activeObject = pcObject;

    // insert in the name map
    d->objectMap[ObjectName] = pcObject;
    // generate object id and add to id map;
    if(!pcObject->_Id) pcObject->_Id = ++d->lastObjectId;
    d->objectIdMap[pcObject->_Id] = pcObject;
    // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);
    // insert in the vector
    d->objectArray.push_back(pcObject);

    pcObject->Label.setValue( ObjectName );

    // mark the object as new (i.e. set status bit 2) and send the signal
    pcObject->setStatus(ObjectStatus::New, true);

    const char *viewType = pcObject->getViewProviderNameOverride();
    pcObject->_pcViewProviderName = viewType?viewType:"";

    signalNewObject(*pcObject);

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        signalTransactionAppend(*pcObject, d->activeUndoTransaction);
    }

    signalActivatedObject(*pcObject);
}

void Document::_addObject(DocumentObject* pcObject, const char* pObjectName)
{
    std::string ObjectName = getUniqueObjectName(pObjectName);
    d->objectMap[ObjectName] = pcObject;
    // generate object id and add to id map;
    if(!pcObject->_Id) pcObject->_Id = ++d->lastObjectId;
    d->objectIdMap[pcObject->_Id] = pcObject;
    d->objectArray.push_back(pcObject);
    // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);

    // do no transactions if we do a rollback!
    if (!d->rollback) {
        // Undo stuff
        _checkTransaction(0,0,__LINE__);
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectDel(pcObject);
    }

    const char *viewType = pcObject->getViewProviderNameOverride();
    pcObject->_pcViewProviderName = viewType?viewType:"";

    // send the signal
    signalNewObject(*pcObject);

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        signalTransactionAppend(*pcObject, d->activeUndoTransaction);
    }

    d->activeObject = pcObject;
    signalActivatedObject(*pcObject);
}

/// Remove an object out of the document
void Document::removeObject(const char* sName)
{
    auto pos = d->objectMap.find(sName);

    // name not found?
    if (pos == d->objectMap.end())
        return;

    if (pos->second->testStatus(ObjectStatus::PendingRecompute)) {
        // TODO: shall we allow removal if there is active udno transaction?
        FC_LOG("pending remove of " << sName << " after recomputing document " << getName());
        pos->second->setStatus(ObjectStatus::PendingRemove,true);
        return;
    }

    _checkTransaction(pos->second,0,__LINE__);

    if(!d->rollback && d->activeUndoTransaction && pos->second->hasChildElement()) {
        // Preserve link group sub object global visibilities. Normally those
        // claimed object should be hidden in global coordinate space. However,
        // when the group is deleted, the user will naturally try to show the
        // children, which may now in the global space. When the parent is
        // undeleted, having its children shown in both the local and global
        // coordinate space is very confusing. Hence, we preserve the visibility
        // here
        for(auto &sub : pos->second->getSubObjects()) {
            if(sub.empty())
                continue;
            if(sub[sub.size()-1]!='.')
                sub += '.';
            auto sobj = pos->second->getSubObject(sub.c_str());
            if(sobj && sobj->getDocument()==this && !sobj->Visibility.getValue())
                d->activeUndoTransaction->addObjectChange(sobj,&sobj->Visibility);
        }
    }

    if (d->activeObject == pos->second)
        d->activeObject = 0;

    // Mark the object as about to be deleted
    pos->second->setStatus(ObjectStatus::Remove, true);
    if (!d->undoing && !d->rollback) {
        pos->second->unsetupObject();
    }

    signalDeletedObject(*(pos->second));

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        // in this case transaction delete or save the object
        signalTransactionRemove(*pos->second, d->activeUndoTransaction);
    }
    else {
        // if not saved in undo -> delete object
        signalTransactionRemove(*pos->second, 0);
    }

#ifdef USE_OLD_DAG
    if (!d->vertexMap.empty()) {
        // recompute of document is running
        for (std::map<Vertex,DocumentObject*>::iterator it = d->vertexMap.begin(); it != d->vertexMap.end(); ++it) {
            if (it->second == pos->second) {
                it->second = 0; // just nullify the pointer
                break;
            }
        }
    }
#endif //USE_OLD_DAG

    // Before deleting we must nullify all dependent objects
    breakDependency(pos->second, true);

    //and remove the tip if needed
    if (Tip.getValue() && strcmp(Tip.getValue()->getNameInDocument(), sName)==0) {
        Tip.setValue(nullptr);
        TipName.setValue("");
    }

    // do no transactions if we do a rollback!
    std::unique_ptr<DocumentObject> tobedestroyed;
    if (!d->rollback) {
        // Undo stuff
        if (d->activeUndoTransaction) {
            // in this case transaction delete or save the object
            d->activeUndoTransaction->addObjectNew(pos->second);
        }
        else {
            // if not saved in undo -> delete object later
            std::unique_ptr<DocumentObject> delobj(pos->second);
            tobedestroyed.swap(delobj);
            tobedestroyed->setStatus(ObjectStatus::Destroy, true);
        }
    }

    for (std::vector<DocumentObject*>::iterator obj = d->objectArray.begin(); obj != d->objectArray.end(); ++obj) {
        if (*obj == pos->second) {
            d->objectArray.erase(obj);
            break;
        }
    }

    pos->second->setStatus(ObjectStatus::Remove, false); // Unset the bit to be on the safe side
    d->objectIdMap.erase(pos->second->_Id);
    d->objectMap.erase(pos);
}

/// Remove an object out of the document (internal)
void Document::_removeObject(DocumentObject* pcObject)
{
    if (testStatus(Document::Recomputing)) {
        FC_ERR("Cannot delete " << pcObject->getFullName() << " while recomputing");
        return;
    }

    // TODO Refactoring: share code with Document::removeObject() (2015-09-01, Fat-Zer)
    _checkTransaction(pcObject,0,__LINE__);

    auto pos = d->objectMap.find(pcObject->getNameInDocument());

    if(!d->rollback && d->activeUndoTransaction && pos->second->hasChildElement()) {
        // Preserve link group children global visibility. See comments in
        // removeObject() for more details.
        for(auto &sub : pos->second->getSubObjects()) {
            if(sub.empty())
                continue;
            if(sub[sub.size()-1]!='.')
                sub += '.';
            auto sobj = pos->second->getSubObject(sub.c_str());
            if(sobj && sobj->getDocument()==this && !sobj->Visibility.getValue())
                d->activeUndoTransaction->addObjectChange(sobj,&sobj->Visibility);
        }
    }

    if (d->activeObject == pcObject)
        d->activeObject = 0;

    // Mark the object as about to be removed
    pcObject->setStatus(ObjectStatus::Remove, true);
    if (!d->undoing && !d->rollback) {
        pcObject->unsetupObject();
    }
    signalDeletedObject(*pcObject);
    // TODO Check me if it's needed (2015-09-01, Fat-Zer)

    //remove the tip if needed
    if (Tip.getValue() == pcObject) {
        Tip.setValue(nullptr);
        TipName.setValue("");
    }

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        // Undo stuff
        signalTransactionRemove(*pcObject, d->activeUndoTransaction);
        d->activeUndoTransaction->addObjectNew(pcObject);
    }
    else {
        // for a rollback delete the object
        signalTransactionRemove(*pcObject, 0);
        breakDependency(pcObject, true);
    }

    // remove from map
    pcObject->setStatus(ObjectStatus::Remove, false); // Unset the bit to be on the safe side
    d->objectIdMap.erase(pcObject->_Id);
    d->objectMap.erase(pos);

    for (std::vector<DocumentObject*>::iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if (*it == pcObject) {
            d->objectArray.erase(it);
            break;
        }
    }

    // for a rollback delete the object
    if (d->rollback) {
        pcObject->setStatus(ObjectStatus::Destroy, true);
        delete pcObject;
    }
}

void Document::breakDependency(DocumentObject* pcObject, bool clear)
{
    // Nullify all dependent objects
    PropertyLinkBase::breakLinks(pcObject,d->objectArray,clear);
}

std::vector<DocumentObject*> Document::copyObject(
    const std::vector<DocumentObject*> &objs, bool recursive)
{
    std::vector<DocumentObject*> deps;
    if(!recursive)
        deps = objs;
    else
        deps = getDependencyList(objs,DepNoXLinked);

    if(!isSaved() && PropertyXLink::hasXLink(deps))
        throw Base::RuntimeError(
                "Document must be saved at least once before link to external objects");
        
    MergeDocuments md(this);
    // if not copying recursively then suppress possible warnings
    md.setVerbose(recursive);

    unsigned int memsize=1000; // ~ for the meta-information
    for (std::vector<App::DocumentObject*>::iterator it = deps.begin(); it != deps.end(); ++it)
        memsize += (*it)->getMemSize();

    // if less than ~10 MB
    bool use_buffer=(memsize < 0xA00000);
    QByteArray res;
    try {
        res.reserve(memsize);
    }
    catch (const Base::MemoryException&) {
        use_buffer = false;
    }

    if (use_buffer) {
        Base::ByteArrayOStreambuf obuf(res);
        std::ostream ostr(&obuf);
        exportObjects(deps, ostr);

        Base::ByteArrayIStreambuf ibuf(res);
        std::istream istr(0);
        istr.rdbuf(&ibuf);
        return md.importObjects(istr);
    } else {
        static Base::FileInfo fi(App::Application::getTempFileName());
        Base::ofstream ostr(fi, std::ios::out | std::ios::binary);
        exportObjects(deps, ostr);
        ostr.close();

        Base::ifstream istr(fi, std::ios::in | std::ios::binary);
        return md.importObjects(istr);
    }
}

std::vector<App::DocumentObject*> 
Document::importLinks(const std::vector<App::DocumentObject*> &objArray)
{
    std::set<App::DocumentObject*> objSet;
    std::vector<App::DocumentObject*> objs;
    for(auto obj : objArray) {
        if(!obj || !obj->getNameInDocument() || obj->getDocument()!=this)
            continue;
        std::vector<App::Property*> props;
        obj->getPropertyList(props);
        for(auto prop : props) {
            auto link = dynamic_cast<App::PropertyLinkBase*>(prop);
            if(!link || prop->testStatus(Property::Immutable) || obj->isReadOnly(prop))
                continue;
            for(auto linked : link->linkedObjects()) {
                if(linked && linked->getNameInDocument() && 
                    linked->getDocument()!=this && 
                    objSet.insert(linked).second)
                {
                    objs.push_back(linked);
                }
            }
        }
    }

    objs = App::Document::getDependencyList(objs);
    if(objs.empty()) {
        FC_ERR("nothing to import");
        return objs;
    }

    Base::FileInfo fi(App::Application::getTempFileName());
    {
        // save stuff to temp file
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        MergeDocuments mimeView(this);
        exportObjects(objs, str);
        str.close();
    }
    Base::ifstream str(fi, std::ios::in | std::ios::binary);
    MergeDocuments mimeView(this);
    objs = mimeView.importObjects(str);
    str.close();
    fi.deleteFile();

    const auto &nameMap = mimeView.getNameMap();

    // First, find all link type properties that needs to be changed
    std::map<App::Property*,std::unique_ptr<App::Property> > propMap;
    std::vector<App::Property*> propList;
    for(auto obj : objArray) {
        propList.clear();
        obj->getPropertyList(propList);
        for(auto prop : propList) {
            auto linkProp = dynamic_cast<PropertyLinkBase*>(prop);
            if(linkProp && !prop->testStatus(Property::Immutable) && !obj->isReadOnly(prop)) {
                auto copy = linkProp->CopyOnImportExternal(nameMap);
                if(copy)
                    propMap[linkProp].reset(copy);
            }
        }
    }

    // Then change them in one go. Note that we don't make change in previous
    // loop, because a changed link property may break other depending link
    // properties, e.g. a link sub refering to some sub object of an xlink, If
    // that sub object is imported with a different name, and xlink is changed
    // before this link sub, it will break.
    for(auto &v : propMap) 
        v.first->Paste(*v.second);

    return objs;
}

DocumentObject* Document::moveObject(DocumentObject* obj, bool recursive)
{
    if(!obj)
        return 0;
    Document* that = obj->getDocument();
    if (that == this)
        return 0; // nothing todo

    // True object move without copy is only safe when undo is off on both
    // documents.
    if(!recursive && !d->iUndoMode && !that->d->iUndoMode) {
        // all object of the other document that refer to this object must be nullified
        that->breakDependency(obj, false);
        std::string objname = getUniqueObjectName(obj->getNameInDocument());
        that->_removeObject(obj);
        this->_addObject(obj, objname.c_str());
        obj->setDocument(this);
        return obj;
    }

    std::vector<App::DocumentObject*> deps;
    if(recursive) 
        deps = getDependencyList({obj},DepNoXLinked|DepSort);
    else
        deps.push_back(obj);

    auto objs = copyObject(deps,false);
    if(objs.empty()) 
        return 0;
    // Some object may delete its children if deleted, so we collect the IDs
    // or all depdending objects for saftey reason.
    std::vector<int> ids;
    ids.reserve(deps.size());
    for(auto o : deps)
        ids.push_back(o->getID());

    // We only remove object if it is the moving object or it has no
    // depending objects, i.e. an empty inList, which is why we need to
    // iterate the depending list backwards.
    for(auto iter=ids.rbegin();iter!=ids.rend();++iter) {
        auto o = that->getObjectByID(*iter);
        if(!o) continue;
        if(iter==ids.rbegin()
                || o->getInList().empty())
            that->removeObject(o->getNameInDocument());
    }
    return objs.back();
}

DocumentObject * Document::getActiveObject(void) const
{
    return d->activeObject;
}

DocumentObject * Document::getObject(const char *Name) const
{
    auto pos = d->objectMap.find(Name);

    if (pos != d->objectMap.end())
        return pos->second;
    else
        return 0;
}

DocumentObject * Document::getObjectByID(long id) const
{
    auto it = d->objectIdMap.find(id);
    if(it!=d->objectIdMap.end())
        return it->second;
    return 0;
}


// Note: This method is only used in Tree.cpp slotChangeObject(), see explanation there
bool Document::isIn(const DocumentObject *pFeat) const
{
    for (auto o = d->objectMap.begin(); o != d->objectMap.end(); ++o) {
        if (o->second == pFeat)
            return true;
    }

    return false;
}

const char * Document::getObjectName(DocumentObject *pFeat) const
{
    for (auto pos = d->objectMap.begin();pos != d->objectMap.end();++pos) {
        if (pos->second == pFeat)
            return pos->first.c_str();
    }

    return 0;
}

std::string Document::getUniqueObjectName(const char *Name) const
{
    if (!Name || *Name == '\0')
        return std::string();
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    auto pos = d->objectMap.find(CleanName);

    if (pos == d->objectMap.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        // remove also trailing digits from clean name which is to avoid to create lengthy names
        // like 'Box001001'
        if (!testStatus(KeepTrailingDigits)) {
            std::string::size_type index = CleanName.find_last_not_of("0123456789");
            if (index+1 < CleanName.size()) {
                CleanName = CleanName.substr(0,index+1);
            }
        }

        std::vector<std::string> names;
        names.reserve(d->objectMap.size());
        for (pos = d->objectMap.begin();pos != d->objectMap.end();++pos) {
            names.push_back(pos->first);
        }
        return Base::Tools::getUniqueName(CleanName, names, 3);
    }
}

std::string Document::getStandardObjectName(const char *Name, int d) const
{
    std::vector<App::DocumentObject*> mm = getObjects();
    std::vector<std::string> labels;
    labels.reserve(mm.size());

    for (std::vector<App::DocumentObject*>::const_iterator it = mm.begin(); it != mm.end(); ++it) {
        std::string label = (*it)->Label.getValue();
        labels.push_back(label);
    }
    return Base::Tools::getUniqueName(Name, labels, d);
}

std::vector<DocumentObject*> Document::getDependingObjects() const
{
    return getDependencyList(d->objectArray);
}

const std::vector<DocumentObject*> &Document::getObjects() const
{
    return d->objectArray;
}


std::vector<DocumentObject*> Document::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> Objects;
    for (std::vector<DocumentObject*>::const_iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(typeId))
            Objects.push_back(*it);
    }
    return Objects;
}

std::vector< DocumentObject* > Document::getObjectsWithExtension(const Base::Type& typeId, bool derived) const {

    std::vector<DocumentObject*> Objects;
    for (std::vector<DocumentObject*>::const_iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if ((*it)->hasExtension(typeId, derived))
            Objects.push_back(*it);
    }
    return Objects;
}


std::vector<DocumentObject*> Document::findObjects(const Base::Type& typeId, const char* objname) const
{
    boost::regex rx(objname);
    boost::cmatch what;
    std::vector<DocumentObject*> Objects;
    for (std::vector<DocumentObject*>::const_iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(typeId)) {
            if (boost::regex_match((*it)->getNameInDocument(), what, rx))
                Objects.push_back(*it);
        }
    }
    return Objects;
}

int Document::countObjectsOfType(const Base::Type& typeId) const
{
    int ct=0;
    for (auto it = d->objectMap.begin(); it != d->objectMap.end(); ++it) {
        if (it->second->getTypeId().isDerivedFrom(typeId))
            ct++;
    }

    return ct;
}

PyObject * Document::getPyObject(void)
{
    return Py::new_reference_to(DocumentPythonObject);
}

std::vector<App::DocumentObject*> Document::getRootObjects() const
{
    std::vector < App::DocumentObject* > ret;

    for (auto objectIt : d->objectArray) {
        if (objectIt->getInList().empty())
            ret.push_back(objectIt);
    }

    return ret;
}

void DocumentP::findAllPathsAt(const std::vector <Node> &all_nodes, size_t id,
                                std::vector <Path> &all_paths, Path tmp)
{
    if (std::find(tmp.begin(), tmp.end(), id) != tmp.end()) {
        Path tmp2(tmp);
        tmp2.push_back(id);
        all_paths.push_back(tmp2);
        return; // a cycle
    }

    tmp.push_back(id);
    if (all_nodes[id].empty()) {
        all_paths.push_back(tmp);
        return;
    }

    for (size_t i=0; i < all_nodes[id].size(); i++) {
        Path tmp2(tmp);
        findAllPathsAt(all_nodes, all_nodes[id][i], all_paths, tmp2);
    }
}

std::vector<std::list<App::DocumentObject*> >
Document::getPathsByOutList(const App::DocumentObject* from, const App::DocumentObject* to) const
{
    std::map<const DocumentObject*, size_t> indexMap;
    for (size_t i=0; i<d->objectArray.size(); ++i) {
        indexMap[d->objectArray[i]] = i;
    }

    std::vector <Node> all_nodes(d->objectArray.size());
    for (size_t i=0; i<d->objectArray.size(); ++i) {
        DocumentObject* obj = d->objectArray[i];
        std::vector<DocumentObject*> outList = obj->getOutList();
        for (auto it : outList) {
            all_nodes[i].push_back(indexMap[it]);
        }
    }

    std::vector<std::list<App::DocumentObject*> > array;
    if (from == to)
        return array;

    size_t index_from = indexMap[from];
    size_t index_to = indexMap[to];
    Path tmp;
    std::vector<Path> all_paths;
    DocumentP::findAllPathsAt(all_nodes, index_from, all_paths, tmp);

    for (std::vector<Path>::iterator it = all_paths.begin(); it != all_paths.end(); ++it) {
        Path::iterator jt = std::find(it->begin(), it->end(), index_to);
        if (jt != it->end()) {
            std::list<App::DocumentObject*> path;
            for (Path::iterator kt = it->begin(); kt != jt; ++kt) {
                path.push_back(d->objectArray[*kt]);
            }

            path.push_back(d->objectArray[*jt]);
            array.push_back(path);
        }
    }

    // remove duplicates
    std::sort(array.begin(), array.end());
    array.erase(std::unique(array.begin(), array.end()), array.end());

    return array;
}

bool Document::mustExecute() const
{
    if(PropertyXLink::hasXLink(this)) {
        bool touched = false;
        _buildDependencyList(d->objectArray,false,0,0,0,&touched);
        return touched;
    }

    for (std::vector<DocumentObject*>::const_iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        if ((*It)->isTouched() || (*It)->mustExecute()==1)
            return true;
    return false;
}

