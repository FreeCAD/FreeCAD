/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CLUSTERGRAPH_HPP
#define CLUSTERGRAPH_HPP

#include <map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <boost/mpl/transform.hpp>
#include <boost/mpl/find.hpp>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/container/vector.hpp>

#include "property.hpp"

#include <Eigen/Core>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

/** @addtogroup Core
 * @{
 * */

/** @addtogroup ClusterGraph
 * @{*/

namespace details {

/** @addtogroup Metafunctions
 * @{*/

/**
 * @brief Creates a fusion::vector of boost shared_ptr's from the given types
 *
 * Creates a shared pointer sequence (sps) of the supplied types by converting them to
 * boost::shared_ptr's first and creating a fusion::vector of all pointers afterwards which can be
 * accessed by the type typedef. Usage: @code sps<types>::type @endcode
 *
 * @tparam seq the mpl::sequence with the types to convert to shared_ptr's
 **/
template<typename seq>
struct sps { //shared_ptr sequence
    typedef typename mpl::transform<seq, boost::shared_ptr<mpl::_1> >::type spv;
    typedef typename fusion::result_of::as_vector<spv>::type type;
};
/**@}*/

//Define vertex and edge properties which are always added for use in the boost graph library algorithms
//which are used in the ClusterGraph implementation
typedef mpl::vector1<vertex_index_prop> bgl_v_props;
typedef mpl::vector1<edge_index_prop> bgl_e_props;

typedef boost::adjacency_list_traits<boost::listS, boost::listS, boost::undirectedS> list_traits;


/**
 * @brief A type to be used as identifier for vertices and edges
 *
 * Vertices and edges need to be identified in a stable(safe/load), unique(over multiple clusters) and
 * comparable manner. The bgl vertex and edge discriptors don't fulfill this need as they have a direct
 * relation to the graphs storage. Therefore they change value on moving entitiys to different clusters or
 * clone actions. This class is used to overcome this problem.
 **/
typedef int universalID;

/**
 * @brief Generator for unique identifiers
 *
 * The universalID used to identify vertices and edges globaly need to be unique and therefore can't be
 * created at good will. This generator creates universalID's in a incremental manner and is intended to
 * to be shared between all graphs of a system, so that all created ID's are unique.
 **/
struct IDgen {
    universalID* counter;

    IDgen() {
        counter = new universalID(10);
    };
    IDgen(universalID id) {
        counter = new universalID(id);
    };
    ~IDgen() {
        delete counter;
    };
    /**
     * @brief Generates a new unique ID
     *
     * @return :details::universalID
     **/
    universalID generate() {
        return ++ (*counter);
    };
    /**
     * @brief Returns the amount if generated ID's
     *
     * As universalID's are integers the returned count is a ID and can therefore also be used as the last
     * created ID.
     *
     * @return :details::universalID
     **/
    universalID count() {
        return (*counter);
    };
    /**
     * @brief Set the current value for incremental creation
     *
     * ID's are created incrementaly and if a specific startingpoint is wished it can be set here by
     * supplying the last created ID or the amount of totaly created ID's
     *
     * @param id The last created ID
     * @return void
     **/
    void setCount(universalID id) {
        *counter = id;
    };
};

/**
 * @brief Exception thrown from the graph at any occurring error
 **/
struct cluster_error : virtual boost::exception {};

/**
 * @brief Pointer type to share a common ID generator @ref IDgen
 **/
typedef boost::shared_ptr<IDgen> IDpointer;

}

/** @name Descriptors */
/**@{
 * @brief Identifier for local vertices
 *
 * The boost graph library works with identifiers for vertices which directly relate to there storage.
 * Therefore they can be used only in the relevant cluster, they are local. These are the descriptors
 * which need to be used for all bgl algorithms.
 **/
typedef details::list_traits::vertex_descriptor 	LocalVertex;

/**
 * @brief Identifier for local edge
 *
 * The boost graph library works with identifiers for edges which directly relate to there storage.
 * Therefore they can be used only in the relevant cluster, they are local. These are the descriptors
 * which need to be used for all bgl algorithms.
 **/
typedef details::list_traits::edge_descriptor 		LocalEdge;

/**
 * @brief Identifier for global vertex
 *
 * To overcome the locality of the bgl vertex descriptors a global alternative is introduced. This descriptor
 * is unique over clusters and stable on moves and clones.
 **/
typedef details::universalID 				GlobalVertex;

/**
 * @brief Identifier for global edge
 *
 * To overcome the locality of the bgl edge discriptors a global alternative is introduced. This descriptor
 * is unique over clusters and stable on moves and clones. It holds it's source and target also as global
 * descriptors of type GlobalVertex and has a unique ID in form of a universalID assigned.
 **/
struct 	GlobalEdge {
    GlobalVertex source;
    GlobalVertex target;
    details::universalID ID;

    bool operator== (const GlobalEdge& second) const {
        return ID == second.ID;
    };
    bool operator!= (const GlobalEdge& second) const {
        return ID != second.ID;
    };
    bool valid() {
        return ID > 9;
    };
};
/**@}*/


/**
 * @brief A graph that can be stacked in a tree-like manner without losing it connections
 *
 * This is basically a boost adjacency_list with single linked lists 'listS' as storage for vertices and
 * edges. The edges are undirected. This allows to use all boost graph algorithms and provides therefore
 * an comprehensive way for analysing and manipulating its content. It further extends the class with the
 * possibility to cluster its content and to add properties and objects to all entities. For more
 * information, see the module ClusterGraph
 *
 * @tparam edge_prop a mpl::vector with properties which are added to local edges
 * @tparam vertex_prop a mpl::vector with properties which are added to vertices
 * @tparam cluster_prop a mpl::vector with properties which are added to all clusters
 * @tparam objects a mpl::vector with all object types which shall be stored at vertices and edges
 **/
template< typename edge_prop, typename vertex_prop, typename cluster_prop, typename objects>
class ClusterGraph : public boost::adjacency_list < boost::listS, boost::listS,
    boost::undirectedS,
    fusion::vector < GlobalVertex,
    typename details::pts< typename details::ensure_properties<vertex_prop, details::bgl_v_props>::type >::type,
    typename details::sps<objects>::type > ,
    fusion::vector < typename details::pts< typename details::ensure_properties<edge_prop, details::bgl_e_props>::type >::type,
    std::vector< fusion::vector< typename details::sps<objects>::type, GlobalEdge > > > > ,
public PropertyOwner<typename details::ensure_property<cluster_prop, changed_prop>::type>,
public boost::noncopyable,
    public boost::enable_shared_from_this<ClusterGraph<edge_prop, vertex_prop, cluster_prop, objects> > {

public:
    /**
     * @brief mpl::vector with all edge properties
     *
     * The edge properties supplied as template argument to the ClusterGraph are extended with graph
     * specific properties, for example a edge_index_prop. These extra properties are intendet to be
     * used with boost graph algorithms as property maps. They need to be in specefied by the ClusterGraph
     * as they are used within it's implementation. If the graph specific properties are already a part
     * of the given property sequence, nothing happens, they are not added twice.
     **/
    typedef typename details::ensure_properties<edge_prop, details::bgl_e_props>::type   edge_properties;
    /**
     * @brief mpl::vector with all vertex properties
     *
     * The vertex properties supplied as template argument to the ClusterGraph are extended with graph
     * specific properties as vertex_index_prop. These extra properties are intendet to be
     * used with boost graph algorithms as property maps. They need to be in specefied by the ClusterGraph
     * as they are used within it's implementation.If the graph specific properties are already a part
     * of the given property sequence, nothing happens, they are not added twice.
     **/
    typedef typename details::ensure_properties<vertex_prop, details::bgl_v_props>::type vertex_properties;

    /**
     * @brief The property bundle for GlobalEdges
     *
     * A local edge in a cluster can hold multiple gloabal ones. Therefor we need an extra bundle for
     * the GlobalEdges. This bundle holds the objects which are added to that global edge and it's identifier.
     * Note that global edges don't have properties, these are only for local ones.
     **/
    typedef fusion::vector< typename details::sps<objects>::type, GlobalEdge > edge_bundle_single;
    /**
     * @brief The property bundle for local edges
     *
     * Local edges can hold multiple global ones, we therefore need a std::vector of global edges. As
     * they are fully described by a edge_bundle_single we store those. Also local edges can have properties,
     * so store a fusion sequence of them too.
     **/
    typedef fusion::vector< typename details::pts<edge_properties>::type, std::vector< edge_bundle_single > > edge_bundle;
    /**
     * @brief Iteator to access all edge_bundle_single stored in a edge_bundle
     **/
    typedef typename std::vector< edge_bundle_single >::iterator edge_single_iterator;
    /**
     * @brief Property bundle for local vertices
     *
     * This bundle is simpler than the edge one, as every vertex has on single bundle. We therefore
     * store the global descriptor for identification, the fusion sequence with the properties and
     * the objects all in one bundle.
     **/
    typedef fusion::vector < GlobalVertex, typename details::pts<vertex_properties>::type,
            typename details::sps<objects>::type > vertex_bundle;

    /**
     * @brief The adjacency_list type ClusterGraph inherited from
     **/
    typedef boost::adjacency_list < boost::listS, boost::listS,
            boost::undirectedS, vertex_bundle, edge_bundle > Graph;

    typedef boost::enable_shared_from_this<ClusterGraph<edge_prop, vertex_prop, cluster_prop, objects> > sp_base;

    //if changed_prop is not a property we have to add it now
    typedef typename details::ensure_property<cluster_prop, changed_prop>::type cluster_properties;

    typedef typename boost::graph_traits<Graph>::vertex_iterator   local_vertex_iterator;
    typedef typename boost::graph_traits<Graph>::edge_iterator     local_edge_iterator;
    typedef typename boost::graph_traits<Graph>::out_edge_iterator local_out_edge_iterator;

    typedef std::map<LocalVertex, boost::shared_ptr<ClusterGraph> > ClusterMap;


    struct global_extractor  {
        typedef GlobalEdge& result_type;
        template<typename T>
        result_type operator()(T& bundle) const;
    };
    
    struct global_vertex_extractor  {
        typedef GlobalVertex result_type;
        ClusterGraph& graph;
        global_vertex_extractor(ClusterGraph& g);
        result_type operator()(LocalVertex& v) const;
    };

    template<typename Obj>
    struct object_extractor  {

        typedef boost::shared_ptr<Obj> base_type;
        typedef base_type& result_type;
        typedef typename mpl::find<objects, Obj>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));

        result_type operator()(vertex_bundle& bundle) const;
        result_type operator()(edge_bundle_single& bundle) const;
    };

    //iterators
    /**
     * @brief Iterator for global edge descriptors \ref GlobalEdge
     **/
    typedef boost::transform_iterator<global_extractor, edge_single_iterator> global_edge_iterator;

    /**
     * @brief Iterator for global vertex descriptor \ref GlobalVertex
     **/
    typedef boost::transform_iterator<global_vertex_extractor, local_vertex_iterator> global_vertex_iterator;

    /**
     * @brief Iterator for objects of given type
     *
     * Allows to iterate over all objects of given type, dereferencing gives the boost::shared_ptr<Obj>
     *
     * @tparam Obj the object type to iterate over
     **/
    template<typename Obj>
    struct object_iterator : public boost::transform_iterator<object_extractor<Obj>, edge_single_iterator> {
        object_iterator(edge_single_iterator it, object_extractor<Obj> f)
            : boost::transform_iterator<object_extractor<Obj>, edge_single_iterator> (it, f) {};
    };

    /**
     * @brief Iterator for clusters
     *
     * Allows to iterate over all subclusters.
     **/
    typedef typename ClusterMap::iterator cluster_iterator;
    /**
     * @brief Const equivalent to \ref cluster_iterator
     **/
    typedef typename ClusterMap::const_iterator const_cluster_iterator;

    /**
     * @brief Basic constructor
     *
     * This constructor creates a empty cluster with a new ID generator. This is to be used on initial
     * clustergraph creation, so only for the very first cluster.
     **/
    ClusterGraph() : m_id(new details::IDgen) {};

    /**
     * @brief Dependent constructor
     *
     * This constructor creates a new cluster, but uses the given cluster as parent. It will therefore
     * create a tree-like relationship. Be aware, that the new cluster is not added to the parents
     * subcluster list, that has to be done manualy. The new cluster shares the parents ID generator.
     *
     * @param g the parent cluster graph
     **/
    ClusterGraph(boost::shared_ptr<ClusterGraph> g) : m_parent(g), m_id(new details::IDgen) {
        if(g)
            m_id = g->m_id;
    };

    ~ClusterGraph() {};

    /**
     * @brief Copys the Clustergraph into a new one
     *
     * Copys this cluster and all subclusters into the give one, which is cleared bevore copying. Be
     * aware that all objects and properties are only copied, and as some are shared pointers (namely
     * all objects) you may have to clone them. If needed this can be done with the supplied functor,
     * which receives all copied objects to his function operator which returns the new object.
     * @param into The Graph that should be a copy of this
     * @param functor The function objects which gets the graph objects and returns the ones for the
     * copied graph
     */
    template<typename Functor>
    void copyInto(boost::shared_ptr<ClusterGraph> into, Functor& functor) const;

    /**
     * @brief Compare by address, not by content
     * @param other the cluster to compare with
     * @return bool if this is the same cluster in memory
     **/
    template<typename T>
    bool operator== (const T& other) const;

    /**
     * @brief Compare by address, not by content
     * @param other the cluster to compare with
     * @return bool if this is the not same cluster in memory
     **/
    template<typename T>
    bool operator!= (const T& other) const;

    /**
     * @brief Set different behaviour for changed markers
     *
     * Some methods of the ClusterGraph set it's changed_prop to true. Thats sensible, as they change
     * the graph. However, there are situations where you want to use the methods but don't want the change
     * marked. For example recreations while cloning. This method can be used to disable the changed setting.
     * @param on Turn change markers on or of
     * @return void
     **/
    void setCopyMode(bool on);

    //Make sure the compiler finds the base class setters even with equal named functions in this class
    using PropertyOwner<cluster_properties>::getProperty;
    using PropertyOwner<cluster_properties>::setProperty;

    /**
     * @brief Set the property of a owned cluster
     *
     * Makes it easy to set a property of a subcluster without retrieving it first
     *
     * @tparam P the property type which shall be set
     * @param v the local vertex which describes the subcluster
     **/
    template<typename P>
    typename P::type& getSubclusterProperty(LocalVertex v);

    /**
    * @brief Mark if the cluster was changed
    *
    * @return void
    **/
    void setChanged();


    /* *******************************************************
     * Subclustering
     * *******************************************************/

    /**
     * @brief Creates a new subcluster
     *
     * As clusters can be stacked in a tree like manner, this function can be used to create new
     * children. It automatically adds it to the subcluster list and adds it to the graph. The new
     * subcluster is fully defined by its object and the vertex descriptor which is it's position
     * in the current cluster.
     *
     * @return :pair< boost::shared_ptr< ClusterGraph >, LocalVertex > Subcluster and its descriptor
     **/
    std::pair<boost::shared_ptr<ClusterGraph>, LocalVertex> createCluster();

    /**
     * @brief Returns the parent cluster
     *
     * In the stacked cluster hirarchy most clusters have a parent which can be accessed with this function.
     * However, the toplevel cluster dos nothave a parent and a empty shared_ptr is returned.
     *
     * @return :shared_ptr< ClusterGraph > the parent cluster or empty pointer
     **/
    boost::shared_ptr<ClusterGraph> parent();

    /**
     * @brief const version of \ref parent()
     *
     * @return :shared_ptr< ClusterGraph >
     **/
    const boost::shared_ptr<ClusterGraph> parent() const;

    /**
     * @brief Is this the toplevel cluster?
     *
     * @return bool if it is
     **/
    bool isRoot() const;

    /**
     * @brief Returns the toplevel cluster
     *
     * @return :shared_ptr< ClusterGraph >
     **/
    boost::shared_ptr<ClusterGraph>	 root();

    /**
     * @brief const equivalent of \ref root()
     *
     * @return :shared_ptr< ClusterGraph >
     **/
    const boost::shared_ptr<ClusterGraph> root() const;

    /**
     * @brief Iterators for all subclusters
     *
     * A pair with two \ref cluster_iterator is returned which point to the first cluster and
     * to one after the last. #this allows full iteration over all subclusters
     *
     * @return :pair< cluster_iterator, cluster_iterator >
     **/
    std::pair<cluster_iterator, cluster_iterator> clusters();

    /**
     * @brief const equivalent to \ref clusters()
     *
     * @return :pair< const_cluster_iterator, const_cluster_iterator >
     **/
    std::pair<const_cluster_iterator, const_cluster_iterator> clusters() const;

    /**
     * @brief The amount of all subclusters
     *
     * @return :size_t
     **/
    std::size_t numClusters() const;

    /**
     * @brief Check if this vertex is a cluster
     *
     * A subcluster is added as normal vertex to the parent cluster. There is no way to distinguish
     * between clusters and normal vertices with global or local descriptors only. Therefore this
     * function can be used to get information about the type. If it is a cluster, it can be accessed
     * with \ref getVertexCluster
     *
     * @param v The vertex to be checked
     * @return bool is cluster or not
     **/
    bool isCluster(const dcm::LocalVertex v) const;

    /**
     * @brief Get the cluster corresponding the discriptor
     *
     * A subcluster is added as normal vertex to the parent cluster. There is no way to access
     * the clusters object with global or local descriptors only. Therefore this
     * function can be used to get the object belonging to the descriptor. If the vertex is not
     * a cluster an empty pointer is returned.
     *
     * @param v The vertex for which the cluster is wanted
     * @return boost::shared_ptr<ClusterGraph> the coresponding cluster orempty pointer
     **/
    boost::shared_ptr<ClusterGraph> getVertexCluster(LocalVertex v);

    /**
     * @brief Get the vertex descrptor which descripes the clusters position in the graph
     *
     * This function is the inverse to \ref getVertexCluster
     *
     * @param g the graph for which the vertex is searched
     * @return :LocalVertex
     **/
    LocalVertex	getClusterVertex(boost::shared_ptr<ClusterGraph> g);

    /**
     * @brief Convenience function for \ref removeCluster
     **/
    template<typename Functor>
    void removeCluster(boost::shared_ptr<ClusterGraph> g, Functor& f);
    /**
     * @brief Convenience function for \ref removeCluster
     **/
    void removeCluster(boost::shared_ptr<ClusterGraph> g);
    /**
     * @brief Delete all subcluster
     *
     * @return void
     **/

    void clearClusters();

    /**
     * @brief Remove a subcluster and applys the functor to all removed edges and vertices
     *
     * All downstream elements of the local vertex v will be removed after the functor is applied to there
     * edges and vertices. Note that the LocalVertex which represents the cluster to delete is not passed
     * to the functor. When ever the cluster is changed it will be passed to the functor, so that it need
     * to have three overloads: operator()(GlobalEdge), operator()(GlobalVertex), operator()(ClusterGraph&)
     *
     * @param v Local vertex which is a cluster and which should be deleted
     * @param f Functor to apply on all graph elements
     */
    template<typename Functor>
    void removeCluster(LocalVertex v, Functor& f);
    void removeCluster(LocalVertex v);

protected:
    template<typename Functor>
    void remove_vertices(Functor& f, bool recursive = false);


    /* *******************************************************
    * Creation Handling
    * *******************************************************/

public:
    /**
     * @brief Add a vertex to the local cluster
     *
     * @return fusion::vector<LocalVertex, GlobalVertex> the local and global vertex descriptor
     **/
    fusion::vector<LocalVertex, GlobalVertex> addVertex();

    /**
     * @brief Add a vertex to the local cluster with given global identifier
     *
     * Sometimes it is needed to add a vertex with given global identifier. As the global vertex can not
     * be changed after creation, this method can be used to specify the global vertex by which this 
     * graph vertex can be identified. The given global vertex is not checked, you need to ensure that 
     * it is a unique id or the already existing vertex is returned.
     * The ID generator is changed so that it creates only identifier bigger than v.
     * 
     * @return fusion::vector<LocalVertex, GlobalVertex> the local and global vertex descriptor
     **/
    fusion::vector<LocalVertex, GlobalVertex> addVertex(GlobalVertex v);
    
    /**
     * @brief Iterators of all global vertices in this cluster
     *
     * Returns the iterator for the first global vertex and the end() iterator as reference for
     * iterating
     *
     * @return std::pair< global_vertex_iterator, global_vertex_iterator > global vertex iterators
     **/
    std::pair<global_vertex_iterator, global_vertex_iterator> globalVertices();

    /**
     * @brief Returns the edge between the local vertices
     *
     * This function is the same as boost::edge(source, target, Graph) and only added for convienience.
     *
     * @param source LocalEdge as edge source
     * @param target LocalEdge as edge target
     * @return std::pair<LocalEdge, bool> with the local edge descriptor if existing. The bool value shows if the
     * edge exists or not
     **/
    std::pair<LocalEdge, bool> edge(LocalVertex source, LocalVertex target);

    /**
     * @brief Add a edge between two vertices, defined by local descriptors.
     *
     * Add an edge that connects the two vertices and in the local clustergraph and assign the GlobalEdge to it. The
     * LocalVertex parameters should not represent a cluster which would result in the functions failure. If there's
     * already a local edge between the vertices a new global edge will be added and returned. Failure will be
     * recocnisable by a false value in the returned type sequence.
     *
     * @param source The first vertex the edge should connect
     * @param target The second vertex the edge should connect
     * @return fusion::vector<LocalEdge, GlobalEdge, success> with the local and global descriptors of the edge and an bool
     * value indicationg the successful creation.
     **/
    fusion::vector<LocalEdge, GlobalEdge, bool> addEdge(LocalVertex source, LocalVertex target);

    /**
     * @brief Add a edge between two vertices, defined by global descriptors.
     *
     * Adds an edge between vertices which are not nesseccarily in this local cluster and have therefore to be
     * identified with global descriptors. The only condition for source and target vertex is that both must be
     * in the local cluster or any of its subclusters. If thats not the case, the function will fail. On success
     * a new GlobalEdge will be created, but not necessarily a local one. If the vertices are in different cluster
     * which are already connected the global edge will be added to this connecting local edge. Thats the one returned
     * in the seqence. Note that it's possible that the local edge belongs to another subcluster and therefore can't be
     * used in the local cluster. This case is indicated by the scope return value.
     *
     * @param source The first vertex the edge should connect
     * @param target The second vertex the edge should connect
     * @return fusion:vector< LocalEdge, GlobalEdge, success, scope > with the new global edge descriptor and the local
     * one where it was added. Success indicates if the function was successful and scope shows the validy of the local
     * descriptor in this cluster (true means the edge is in this cluster).
     **/
    fusion::vector<LocalEdge, GlobalEdge, bool, bool> addEdge(GlobalVertex source, GlobalVertex target);

    fusion::vector<LocalEdge, GlobalEdge, bool, bool> addEdgeGlobal(GlobalVertex source, GlobalVertex target);

    /**
     * @brief Get an iterator to all the global edges hold by this local edge
     *
     * Local edges can hold multiple global ones, for example when they connect at least one cluster. Therefore a direct
     * LocalEdge - GlobalEdge mapping is not possible. Instead you can access all GlobalEdge's hold by this local one in
     * a normal iterating manner.
     *
     * @param e the local edge for which the global descriptors are wanted
     * @return std::pair<begin, end> with the global_edge_iterator's pointing to the vector<GlobalEdge>'s start
     * and end
     **/
    std::pair<global_edge_iterator, global_edge_iterator> getGlobalEdges(LocalEdge e);

    /**
     * @brief Get the count of all global edges
     *
     * Local edges can hold multiple global ones, for example when they connect at least one cluster. To get the
     * number of all global edges in this local one you can use this function.
     *
     * @param e the local edge for which the global descriptors are wanted
     * @return std::pair<begin, end> with the global_edge_iterator's pointing to the vector<GlobalEdge>'s start
     * and end
     **/
    int getGlobalEdgeCount(LocalEdge e);

    /**
     * @brief Get the local edge which holds the specified global edge.
     *
     * Note that GlobalEdge must be in a local edge of this cluster, means the connected vertices must be in this
     * or one of it's subclusters (but not the same). Also if the containing LocalEdge is not in this cluster, but in one of it's
     * subclusters, the function fails and the returned edge is invalid.
     *
     * @param e GlobalEdge for which the containing local one is wanted
     * @return std:pair< LocalEdge, bool > with the containing LocalEdge and a bool indicator if function was successful.
     **/
    std::pair<LocalEdge, bool> getLocalEdge(GlobalEdge e);

    /**
     * @brief Get the local edge which holds the specified global one and the subcluster in which it is valid.
     *
     * The function only fails when the global edge is hold by a local one upstream in the cluster
     * herarchy.
     *
     * @param e GlobalEdge for which the containing local one is wanted
     * @return fusion::vector<LocalEdge, ClusterGraph*, bool> with the containing LocalEdge, the cluster which holds it and a bool indicator if function was successful.
     **/
    fusion::vector<LocalEdge, ClusterGraph*, bool> getLocalEdgeGraph(GlobalEdge e);

    /**
     * @brief Get the GlobalVertex assiociated with this local one.
     *
     * @param v LocalVertex
     * @return GlobalVertex
     **/
    GlobalVertex getGlobalVertex(LocalVertex v) const;

    /**
     * @brief Get the LocalVertex which corresponds to the golab one
     *
     * The GlobalVertex has to be in this cluster or any of it's subclusters. If its in a subcluster, the returned
     * LocalVertex will represent this cluster. If the GlobalVertex is not in this clusters scope the function fails.
     *
     * @param vertex GlobalVertex for which the local one shall be returned
     * @return std::pair< LocalVertex, bool > The LocalVertex containing the global one and an success indicator
     **/
    std::pair<LocalVertex, bool> getLocalVertex(GlobalVertex vertex);

    /**
     * @brief Get the local vertex which holds the specified global one and the subcluster in which it is valid.
     *
     * The function only fails when the global vertex is hold by a local one upstream in the cluster
     * herarchy.
     *
     * @param v GlobalVertex for which the containing local one is wanted
     * @return fusion::vector<LocalVertex, ClusterGraph*, bool> with the containing LocalVertex, the cluster which holds it and a bool indicator if function was successful.
     **/
    fusion::vector<LocalVertex, boost::shared_ptr<ClusterGraph>, bool> getLocalVertexGraph(GlobalVertex v);


    /* *******************************************************
     * Remove Handling
     * *******************************************************/
private:

    template<typename Functor>
    void downstreamRemoveVertex(GlobalVertex v, Functor& f);

    void simpleRemoveEdge(LocalEdge e);


public:
    /**
    * @brief Removes a vertex from the local cluster and applys functor to removed edges
    *
    * Removes the vertex from the local graph and invalidates the global vertex id. Also all edges connecting
    * to this vertex will be removed after the functor was applied to them. The functor needs to implement
    * operato()(GlobalEdge e). Remark that there is no checking done if the vertex is a cluster, so you
    * need to make sure it's not, as removing a clustervertex will not delete the coresponding cluster.
    *
    * @param id Local Vertex which should be removed from the graph
    * @param f functor whose operator(GlobalEdge) is called for every removed edge
    **/
    template<typename Functor>
    void removeVertex(LocalVertex id, Functor& f);
    //no default template arguments for template functions allowed before c++0x, so a little workaround
    void removeVertex(LocalVertex id) ;

    /**
    * @brief Removes a vertex from the cluster or it's subclusters and applys functor to removed edges
    *
    * Removes the vertex from the graph or subclusters and invalidates the global vertex id. Also all edges connecting
    * to this vertex will be removed (upstream and downstream) after the functor was applied to them. The functor
    * needs to implement operato()(LocalEdge edge).
    *
    * @param id Global Vertex which should be removed from the graph
    * @param f functor whose operator(LocalEdge) is called on every removed edge
    **/
    template<typename Functor>
    void removeVertex(GlobalVertex id, Functor& f);
    //no default template arguments for template functions allowed before c++0x, so a little workaround
    void removeVertex(GlobalVertex id);

    /**
    * @brief Removes a global Edge from the cluster or it's subclusters
    *
    * Removes the edge from the graph or subclusters and invalidates the global edge id. If the local edge holds
    * only this global one it will be removed also.
    *
    * @param id Global Edge which should be removed from the graph
    * @return bool indicates if the global id could be removed
    **/
    void removeEdge(GlobalEdge id);

    /**
    * @brief Removes a local edge from the cluster and calls the functor for all removed global edges
    *
    * Removes the edge from the graph and invalidates the global edges. The Functor needs to provide
    * operator()(GlobalEdge). If no functor is needed just use boost::remove_edge.
    *
    * @param id Global Edge which should be removed from the graph
    * @param f functor whoms operator(GlobalEdge) is called
    * @return bool indicates if the global id could be removed
    **/
    template<typename Functor>
    void removeEdge(LocalEdge id, Functor& f);


    /* *******************************************************
     * Object Handling
     * *******************************************************/
public:

    /**
    * @brief Get the desired object at the specified vertex or edge
    *
    * This function allows to access the objects stored in the graph. If no object of the desired type
    * was set before, a empty shared_ptr will be returned. Accessing the object at a local edge is a special
    * case, as it can hold many global edges, each with it's own objetcs. Using a LocalEdge as key will
    * always return the object for the first GlobalEdge.
    *
    * @tparam Obj the object type which shall be returned
    * @param k local or global Vertex/Edge descriptor for which the object is desired
    * @return shared_ptr< Obj > the pointer to the desired object
    **/
    template<typename Obj, typename key>
    boost::shared_ptr<Obj> getObject(key k);

    /**
     * @brief Set a object at the specified vertex or edge
     *
     * Sets the given value at the given key. Note that every entity can hold only one object, so setting
     * a new value resets all other objects which were set before. Setting the object at a local edge is a special
     * case, as it can hold many global edges, each with it's own objects. Using a LocalEdge as key will
     * always set the object for the first GlobalEdge.
     *
     * @tparam Obj the object type which shall be set
     * @param k local or global Vertex/Edge descriptor for which the object should be set
     * @param val the object which should be stored
     * @return void
     **/
    template<typename Obj, typename key>
    void setObject(key k, boost::shared_ptr<Obj> val);

    /**
     * @brief Get iterator range for all GlobalEdge objects hold by this local edge
     *
     * LocalEdge's can hold multiple global ones and the iterators can be used to access a specific object type in
     * all global edges hold by this local edge.
     *
     * @tparam Obj the object type over which it shall be iterated
     * @param k the LocalEdge over which all Objects should be iterated.
     * @return pair< begin, end > the iterator rang from begin (first element) to end (first undefined element)
     **/
    template<typename Obj>
    std::pair< object_iterator<Obj>, object_iterator<Obj> > getObjects(LocalEdge k);

    /**
     * @brief Applys the functor to each occurrence of an object
     *
     * Each valid object of the given type is extractet and passed to the function object. Vertices
     * and edges are searched for valid object pointers, it happens in this order. When a recursive
     * search is specified, all subclusters are searched too, but the cluster is passt to the Functor
     * first. So make sure a function overload for clusters exist in this case.
     *
     * @tparam Obj the object type for which the functor shall be used
     * @param f the functor to which all valid objects get passed to.
     * @param recursive specifies if the subclusters should be searched for objects too
     **/
    template<typename Obj, typename Functor>
    void for_each(Functor& f, bool recursive = false);

    /**
     * @brief Applys the functor to each object
     *
     * Each valid object of any type is extractet and passed to the function object. Vertices
     * and edges are searched for valid object pointers, it happens in this order. When a recursive
     * search is specified, all subclusters are searched too, but the cluster is passt to the Functor
     * first. So make sure a function overload for clusters exist in this case.
     *
     * @param f the functor to which all valid objects get passed to.
     * @param recursive specifies if the subclusters should be searched for objects too
     **/
    template<typename Functor>
    void for_each_object(Functor& f, bool recursive = false);

    /* *******************************************************
     * Property Handling
     * *******************************************************/

    /**
    * @brief Get the desired property at the specified vertex or edge
    *
    * This function allows to access the properties stored in the graph. If no property of the desired type
    * was set before, a default construced will be returned. Accessing the property at a global edge will return
    * the property of the holding local edge.
    *
    * @tparam property the property type which shall be returned
    * @param k local or global Vertex/Edge descriptor for which the property is desired
    * @return property::type& the reference to the desired property
    **/
    template<typename property, typename key>
    typename property::type& getProperty(key k);

    /**
     * @brief Set a property at the specified vertex or edge
     *
     * Sets the given value at the given key. Note that every entity can hold one of each property, as opposed
     * to objects. Setting the property at a local edge is a special case, as it can hold many global edges,
     * each with it's own propertys. Using a LocalEdge as key will always set the property for the first GlobalEdge.
     *
     * @tparam property the property type which shall be set
     * @param k local or global Vertex/Edge descriptor for which the property should be set
     * @param val the property value which should be stored
     * @return void
     **/
    template<typename property, typename key>
    void setProperty(key k, typename property::type val);

    /**
     * @brief recreate the internal index maps for edges and vertices
     *
     * Quite many boost graph algorithms need the indices for vertices and edges which are provided by property
     * maps. As we use list, and not vector, as underlaying storage we don't get that property for free and
     * need to create it ourself. To ease that procedure the internal property vertex_index_prop and edge_index_prop
     * can be used as property maps and can be initialized by calling this function.
     *
     * @return void
     **/
    void initIndexMaps();



    /********************************************************
    * Vertex and Cluster moving
    * *******************************************************/

    /**
     * @brief Move a vertex to a subcluster
     *
     * Overloaded convenience function which fetches the local descriptor for the cluster reference and calls
     * the full parameter equivalent. Both cluster and vertex must be in the local cluster.
     *
     * @param v the LocalVertex to be moved
     * @param cg reference to the subcluster to which v should be moved
     * @return LocalVertex the local descriptor of the moved vertex in the subcluster
     **/
    LocalVertex moveToSubcluster(LocalVertex v, boost::shared_ptr<ClusterGraph> cg);

    /**
     * @brief Move a vertex to a subcluster
     *
     * Overloaded convenience function which fetches the the cluster reference for the local descriptor and calls
     * the full parameter equivalent. Both cluster and vertex must be in the local cluster.
     *
     * @param v the LocalVertex to be moved
     * @param Cluster the local vertex descriptor representing the subcluster to which v should be moved
     * @return LocalVertex the local descriptor of the moved vertex in the subcluster
     **/
    LocalVertex moveToSubcluster(LocalVertex v, LocalVertex Cluster);

    /**
     * @brief Move a vertex to a subcluster
     *
     * This function moves the LocalVertex to the subcluster and reconnects all other vertices and clusters. The
     * moved vertex will hold it's global descriptor but get a new local one assigned (the one returned). The same
     * stands for all edges which use the moved vertex: global descriptors stay the same, but they are moved to new
     * local edges. It's allowed to move cluster vertices with this function.
     * The specified cluster has of course to be a valid and direct subcluster, the move vertex also has to be in the
     * local cluster.
     *
     * @param v the LocalVertex to be moved
     * @param Cluster the local vertex descriptor representing the subcluster to which v should be moved
     * @param cg reference to the subcluster to which v should be moved
     * @return LocalVertex the local descriptor of the moved vertex in the subcluster
     **/
    LocalVertex moveToSubcluster(LocalVertex v, LocalVertex Cluster, boost::shared_ptr<ClusterGraph> cg);


    /**
     * @brief Move a vertex to the parent cluster.
     *
     * This function moves a vertex one step up in the subcluster hierarchie and reconnects all other vertices and clusters.
     * The moved vertex will hold it's global descriptor but get a new local one assigned (the one returned). The same
     * stands for all edges which use the moved vertex: global descriptors stay the same, but they are moved to new
     * local edges. Note that this function is the inverse of moveToSubcluster, and doing Pseudocode:
     * moveToParent(moveToSubcluster(v)) does nothing (only the local descriptor of the moved vertex is
     * different afterwards).
     *
     * @param v Local vertex which should be moved to the parents cluster
     * @return LocalVertex the local descriptor of the moved vertex, valid in the parent cluster only.
     **/
    LocalVertex moveToParent(LocalVertex v);


    /********************************************************
    * Stuff
    * *******************************************************/

    ClusterMap	  m_clusters;
    int test;
    
protected:
    boost::weak_ptr<ClusterGraph> m_parent;
    details::IDpointer 	  m_id;
    bool copy_mode; //no changing itself when copying


    /* Searches the global vertex in all local vertices of this graph, and returns the local
     * one which holds the global vertex. If not successful the local vertex returned will be
     * invalid and the bool parameter will be false. If recursive = true, all subclusters will
     * be searched too, however, if found there the returned local vertex will be the vertex
     * representing the toplevel cluster holding the global vertex in the initial graph.
     * */
    std::pair<LocalVertex, bool> getContainingVertex(GlobalVertex id, bool recursive = true);

    /* Searches the local vertex holding the specified global one in this and all it's subclusters.
     * If found, the holding local vertex and the graph in which it is valid will be returned.
     * */
    fusion::vector<LocalVertex, boost::shared_ptr<ClusterGraph>, bool> getContainingVertexGraph(GlobalVertex id);

    /* Searches the global edge in all local edges of this graph, and returns the local
     * one which holds the global edge. If not successful the local edge returned will be
     * invalid and the bool parameter will be false.
     * */
    std::pair<LocalEdge, bool> getContainingEdge(GlobalEdge id);

    /* Searches the local edge holding the specified global one in this and all it's subclusters.
     * If found, the holding local edge and the graph in which it is valid will be returned.
     * */
    fusion::vector<LocalEdge, ClusterGraph*, bool> getContainingEdgeGraph(GlobalEdge id);

    template<typename functor>
    typename functor::result_type apply_to_bundle(LocalVertex k, functor f);

    template<typename functor>
    typename functor::result_type apply_to_bundle(LocalEdge k, functor f);

    template<typename functor>
    typename functor::result_type apply_to_bundle(GlobalVertex k, functor f);

    template<typename functor>
    typename functor::result_type apply_to_bundle(GlobalEdge k, functor f);

public:
    //may hold cluster properties which have Eigen3 objects and therefore need alignment
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

/** @} */
/** @} */

} //namespace dcm


#ifndef DCM_EXTERNAL_CORE
#include "imp/clustergraph_imp.hpp"
#endif

#endif // CLUSTERGRAPH_HPP




