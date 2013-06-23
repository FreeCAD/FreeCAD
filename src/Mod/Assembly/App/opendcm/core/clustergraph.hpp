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
#include <functional>
#include <iostream>
#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <boost/mpl/transform.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/and.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/algorithm.hpp>

#include "property.hpp"
#include <boost/variant/recursive_variant.hpp>
#include <boost/bind.hpp>

#include <Eigen/Core>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace details {

template<typename seq, typename state>
struct vector_fold : mpl::fold< seq, state,
        mpl::push_back<mpl::_1,mpl::_2> > {};
	
//also add basic properties for graph algorithms like index and color
typedef mpl::vector2<vertex_index_prop, vertex_color_prop> bgl_v_props;
typedef mpl::vector2<edge_index_prop, edge_color_prop> bgl_e_props;

	
typedef boost::adjacency_list_traits<boost::listS, boost::listS, boost::undirectedS> list_traits;
typedef int universalID;

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
    universalID generate() {
        return ++(*counter);
    };
    universalID count() {
        return (*counter);
    };
    void setCount(universalID id) {
        *counter = id;
    };
};

typedef boost::shared_ptr<IDgen> IDpointer;

struct clear_ptr {
    template<typename T>
    void operator()(T& t) const {
        t.reset();
    };
};

template<typename T>
struct sps { //shared_ptr sequence
    typedef typename mpl::transform<T, boost::shared_ptr<mpl::_1> >::type spv;
    typedef typename fusion::result_of::as_vector<spv>::type type;
};

}

typedef details::list_traits::vertex_descriptor 	LocalVertex;
typedef details::list_traits::edge_descriptor 		LocalEdge;
typedef details::universalID 				GlobalVertex;
struct 	GlobalEdge {
    GlobalVertex source;
    GlobalVertex target;
    details::universalID ID;

    bool operator==(const GlobalEdge& second) const {
        return ID==second.ID;
    };
    bool operator!=(const GlobalEdge& second) const {
        return ID!=second.ID;
    };
    bool valid() {
        return ID>9;
    };
};


template< typename edge_prop, typename vertex_prop, typename cluster_prop, typename objects>
class ClusterGraph : public boost::adjacency_list< boost::listS, boost::listS,
    boost::undirectedS,
    fusion::vector< GlobalVertex, 
    typename details::pts<typename details::vector_fold<vertex_prop, details::bgl_v_props>::type>::type,
    typename details::sps<objects>::type>,
    fusion::vector< typename details::pts<typename details::vector_fold<edge_prop, details::bgl_e_props>::type>::type,
    std::vector< fusion::vector< typename details::sps<objects>::type, GlobalEdge > > > >,
        public boost::noncopyable, 
	public boost::enable_shared_from_this<ClusterGraph<edge_prop, vertex_prop, cluster_prop, objects> > {

public:
    typedef typename details::vector_fold<edge_prop, details::bgl_e_props>::type   edge_properties;
    typedef typename details::vector_fold<vertex_prop, details::bgl_v_props>::type vertex_properties;
    
    typedef fusion::vector< typename details::sps<objects>::type, GlobalEdge > edge_bundle_single;
    typedef fusion::vector< typename details::pts<edge_properties>::type, std::vector< edge_bundle_single > > edge_bundle;
    typedef typename std::vector< edge_bundle_single >::iterator edge_single_iterator;
    typedef fusion::vector< GlobalVertex, typename details::pts<vertex_properties>::type,
            typename details::sps<objects>::type > vertex_bundle;


    typedef boost::adjacency_list< boost::listS, boost::listS,
            boost::undirectedS, vertex_bundle, edge_bundle > Graph;
	    
    typedef boost::enable_shared_from_this<ClusterGraph<edge_prop, vertex_prop, cluster_prop, objects> > sp_base;

    //if changed_prop is not a property we have to add it now
    typedef typename mpl::if_<
    boost::is_same<
    typename mpl::find<cluster_prop, changed_prop>::type,
             typename mpl::end<cluster_prop>::type >,
             typename mpl::push_back<cluster_prop, changed_prop>::type,
             cluster_prop >::type cluster_properties;
	     
    typedef typename details::pts<cluster_properties>::type cluster_bundle;

    typedef typename boost::graph_traits<Graph>::vertex_iterator   local_vertex_iterator;
    typedef typename boost::graph_traits<Graph>::edge_iterator     local_edge_iterator;
    typedef typename boost::graph_traits<Graph>::out_edge_iterator local_out_edge_iterator;

    typedef std::map<LocalVertex,boost::shared_ptr<ClusterGraph> > ClusterMap;

    cluster_bundle m_cluster_bundle;

private:
    struct global_extractor  {
        typedef GlobalEdge& result_type;
        template<typename T>
        result_type operator()(T& bundle) const {
            return fusion::at_c<1>(bundle);
        };
    };
    struct global_vertex_extractor  {
        typedef GlobalVertex result_type;
        ClusterGraph& graph;
        global_vertex_extractor(ClusterGraph& g) : graph(g) {};
        result_type operator()(LocalVertex& v) const {
            return graph.getGlobalVertex(v);
        };
    };

    template<typename Obj>
    struct object_extractor  {

        typedef boost::shared_ptr<Obj> base_type;
        typedef base_type& result_type;
        typedef typename mpl::find<objects, Obj>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<objects>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<objects>::type > >));

        result_type operator()(vertex_bundle& bundle) const {
            return fusion::at<distance>(fusion::at_c<2>(bundle));
        };
        result_type operator()(edge_bundle_single& bundle) const {
            return fusion::at<distance>(fusion::at_c<0>(bundle));
        };
    };

    template<typename prop>
    struct property_extractor  {

        typedef typename prop::type base_type;
        typedef base_type& result_type;

        typedef typename mpl::if_< is_edge_property<prop>, edge_properties, vertex_properties >::type sequence;
        typedef typename mpl::find<sequence, prop>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<sequence>::type, iterator>::type distance;
	typedef typename mpl::if_< is_edge_property<prop>, mpl::int_<0>, mpl::int_<1> >::type pos;

        template< typename seq>
        result_type operator()(seq& b) const {
            return fusion::at<distance>(fusion::at<pos>(b));
        };
    };

    struct edge_copier {
        edge_copier(const ClusterGraph& g1, ClusterGraph& g2)
            : graph1(g1), graph2(g2) { }

        void operator()(LocalEdge e1, LocalEdge e2) const {
            graph2[e2] = graph1[e1];
        }
        const ClusterGraph& graph1;
        ClusterGraph& graph2;
    };

    struct vertex_copier {
        vertex_copier(const ClusterGraph& g1, ClusterGraph& g2)
            : graph1(g1), graph2(g2) { }

        void operator()(LocalVertex v1, LocalVertex v2) const {
            graph2[v2] = graph1[v1];
        }
        const ClusterGraph& graph1;
        ClusterGraph& graph2;
    };

public:
    //iterators
    typedef boost::transform_iterator<global_extractor, edge_single_iterator> global_edge_iterator;
    typedef boost::transform_iterator<global_vertex_extractor, local_vertex_iterator> global_vertex_iterator;

    template<typename Obj>
    struct object_iterator : public boost::transform_iterator<object_extractor<Obj>, edge_single_iterator> {
        object_iterator(edge_single_iterator it, object_extractor<Obj> f)
            : boost::transform_iterator<object_extractor<Obj>,edge_single_iterator>(it, f) {};
    };

    typedef typename ClusterMap::iterator cluster_iterator;
    typedef typename ClusterMap::const_iterator const_cluster_iterator;

    ClusterGraph() : m_id(new details::IDgen) {};
	
    ClusterGraph(boost::shared_ptr<ClusterGraph> g) : m_parent(g), m_id(new details::IDgen) {
        if(g) m_id = g->m_id;
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
    void copyInto(boost::shared_ptr<ClusterGraph> into, Functor& functor) const {

        //lists does not provide vertex index, so we have to build our own (cant use the internal 
	//vertex_index_property as we would need to reset the indices and that's not possible in const graph)
        typedef std::map<LocalVertex, int> IndexMap;
        IndexMap mapIndex;
        boost::associative_property_map<IndexMap> propmapIndex(mapIndex);

        std::pair<local_vertex_iterator, local_vertex_iterator>  vit = boost::vertices(*this);
        for(int c=0; vit.first != vit.second; vit.first++, c++)
            put(propmapIndex, *vit.first, c);

        //first copy all vertices and edges, but be aware that the objects in the new graph
        //are also copys only and point to the old graph. there is a bug in older boost version
        //(<1.5 i belive) that breaks vertex_all propety map for bundled properties, so we
        //have to create our own copie functors
        into->clear();
        vertex_copier vc(*this, *into);
        edge_copier ec(*this, *into);
        boost::copy_graph(*this, *into, boost::vertex_index_map(propmapIndex).vertex_copy(vc).edge_copy(ec));

        //set the IDgen to the same value to avoid duplicate id's in the copied cluster
        into->m_id->setCount(m_id->count());

        //now that we have all vertices we can recreate the subclusters
        std::pair<const_cluster_iterator, const_cluster_iterator> it = clusters();
        for(; it.first!=it.second; it.first++) {
            //create the new Graph
            boost::shared_ptr<ClusterGraph> ng = boost::shared_ptr<ClusterGraph>(new ClusterGraph(into));

            //we already have the new vertex, however, we need to find it
            GlobalVertex gv = getGlobalVertex((*it.first).first);
            LocalVertex  lv = into->getLocalVertex(gv).first;

            //add the new graph to the subclustermap
            into->m_clusters[lv] = ng;

            //copy the subcluster
            (*it.first).second->copyInto(ng, functor);
        }

        //lets see if the objects need special treatment
        into->for_each_object(functor, false);
    };
   
    template<typename T>
    bool operator==(const T& other) const {
        return this == &other;
    };

    template<typename T>
    bool operator!=(const T& other) const {
        return !(this == &other);
    };
    
    void setCopyMode(bool on) {
	copy_mode = on;
    };

    void setChanged() {
        if(!copy_mode)
	  setClusterProperty<changed_prop>(true);
    };

    /* *******************************************************
     * Cluster Property
     * *******************************************************/
    template<typename P>
    typename P::type& getClusterProperty() {
        typedef typename mpl::find<cluster_properties, P>::type iterator;
        typedef typename mpl::distance<typename mpl::begin<cluster_properties>::type, iterator>::type distance;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<cluster_properties>::type > >));
        return fusion::at<distance>(m_cluster_bundle);
    };
    template<typename P>
    typename P::type& getSubclusterProperty(LocalVertex v) {
        return getVertexCluster(v)->template getClusterProperty<P>();
    };

    template<typename P>
    void setClusterProperty(typename P::type p) {
        getClusterProperty<P>() = p;
    };


    /* *******************************************************
     * Subclustering
     * *******************************************************/
    std::pair<boost::shared_ptr<ClusterGraph>, LocalVertex> createCluster() {
        vertex_bundle vp;
        fusion::at_c<0>(vp) = m_id->generate();
        LocalVertex v= boost::add_vertex(vp, *this);
        return std::pair<boost::shared_ptr<ClusterGraph>, LocalVertex>(m_clusters[v] = boost::shared_ptr<ClusterGraph>(new ClusterGraph(sp_base::shared_from_this())), v);
    };

    inline boost::shared_ptr<ClusterGraph> parent() 	{
        return boost::shared_ptr<ClusterGraph>(m_parent);
    };

    inline const boost::shared_ptr<ClusterGraph> parent() const 	{
        return boost::shared_ptr<ClusterGraph>(m_parent);
    };
    bool isRoot() const {
		return m_parent.expired();
    };

    boost::shared_ptr<ClusterGraph>	 root()		{
        return isRoot() ? sp_base::shared_from_this() : parent()->root();
    };

    const boost::shared_ptr<ClusterGraph> root() const    {
        return isRoot() ? sp_base::shared_from_this() : parent()->root();
    };

    std::pair<cluster_iterator, cluster_iterator> clusters() {
        return std::make_pair(m_clusters.begin(), m_clusters.end());
    }
    std::pair<const_cluster_iterator, const_cluster_iterator> clusters() const {
        return std::make_pair(m_clusters.begin(), m_clusters.end());
    }

    std::size_t numClusters() const {
        return m_clusters.size();
    }

    bool isCluster(LocalVertex v) {
        return (m_clusters.find(v) != m_clusters.end());
    };

    boost::shared_ptr<ClusterGraph> getVertexCluster(LocalVertex v) {
        if(isCluster(v))
            return m_clusters[v];
        //TODO:throw if not a cluster
        return sp_base::shared_from_this();
    };

    LocalVertex	getClusterVertex(boost::shared_ptr<ClusterGraph> g) {
        std::pair<cluster_iterator, cluster_iterator> it = clusters();
        for(; it.first!=it.second; it.first++) {
            if((*it.first).second == g)
                return (*it.first).first;
        }
        return LocalVertex();
    };

    template<typename Functor>
    void removeCluster(boost::shared_ptr<ClusterGraph> g, Functor& f) {
        removeCluster(getClusterVertex(g), f);
    };
    void removeCluster(boost::shared_ptr<ClusterGraph> g) {
        placehoder p;
        removeCluster(getClusterVertex(g), p);
    };
    void clearClusters() {
        m_clusters.clear();
    };

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
    void removeCluster(LocalVertex v, Functor& f) {

        typename ClusterMap::iterator it = m_clusters.find(v);
        if(it == m_clusters.end())
            return; //TODO:throw

        std::pair<LocalVertex, boost::shared_ptr<ClusterGraph> > res = *it;

        //apply functor to all vertices and edges in the subclusters
        f(res.second);
        res.second->remove_vertices(f, true);

        //remove from map, delete subcluster and remove vertex
        m_clusters.erase(v);
        boost::clear_vertex(v, *this); //should not be needed, just to ensure it
        boost::remove_vertex(v, *this);
    };
    void removeCluster(LocalVertex v) {
        placehoder p;
        removeCluster(v, p);
    };

protected:
    template<typename Functor>
    void remove_vertices(Functor& f, bool recursive = false) {

        std::pair<local_vertex_iterator, local_vertex_iterator>  vit = boost::vertices(*this);
        //we iterate forward before deleting to not invalidate our iterator
        while(vit.first != vit.second) {
            LocalVertex v = *(vit.first);
            vit.first++;

            if(!isCluster(v)) {
                //let the functor know we remove this vertex
                f(getGlobalVertex(v));
                //need to do this to allow the removal of all relevant edges to this vertex, even upstream
                removeVertex(v, f);
            }
        };

        if(recursive) {
            cluster_iterator cit;
            for(cit=m_clusters.begin(); cit != m_clusters.end(); cit++) {
                f((*cit).second);
                (*cit).second->remove_vertices(f, recursive);
            }
        }
    };



    /* *******************************************************
    * Creation Handling
    * *******************************************************/

public:
    /**
     * @brief Add a vertex to the local cluster
     *
     * @return fusion:vector< LocalVertex, GlobalVertex > with the local and global vertex descriptor
     **/
    fusion::vector<LocalVertex, GlobalVertex> addVertex() {

        vertex_bundle vp;
        fusion::at_c<0>(vp) = m_id->generate();
        LocalVertex v= boost::add_vertex(vp, *this);

        setChanged();
        return fusion::make_vector(v, m_id->count());
    };

    std::pair<global_vertex_iterator, global_vertex_iterator> globalVertices() {
        std::pair<local_vertex_iterator, local_vertex_iterator> res = boost::vertices(*this);
        global_vertex_iterator begin = boost::make_transform_iterator(res.first, global_vertex_extractor(*this));
        global_vertex_iterator end   = boost::make_transform_iterator(res.second, global_vertex_extractor(*this));

        return std::pair<global_vertex_iterator, global_vertex_iterator>(begin, end);
    };

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
    std::pair<LocalEdge, bool> edge(LocalVertex source, LocalVertex target) {
        return boost::edge(source, target, *this);
    };

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
    fusion::vector<LocalEdge, GlobalEdge, bool> addEdge(LocalVertex source, LocalVertex target) {

        //manual edge creation with cluster is not allowed
        if((source==target) || isCluster(source) || isCluster(target))
            return fusion::make_vector(LocalEdge(), GlobalEdge(), false);

        LocalEdge e;
        bool done;
        boost::tie(e,done) = boost::edge(source, target, *this);

        //if done=true the edge alredy existed
        if(!done) boost::tie(e,done) = boost::add_edge(source, target, *this);
        if(!done) return fusion::make_vector(LocalEdge(), GlobalEdge(), false);

        //init the bundle corecctly for new edge
        GlobalEdge global = { fusion::at_c<0>((*this)[source]), fusion::at_c<0>((*this)[target]), m_id->generate() };
        edge_bundle_single s;
        fusion::at_c<1>(s) = global;
        fusion::at_c<1>((*this)[e]).push_back(s);

        setChanged();
        return fusion::make_vector(e, global, true);
    };

    /**
     * @brief Add a edge between two vertices, defined by global descriptors.
     *
     * Adds an edge between vertices which are not nesseccarily in this local cluster and have therefore to be
     * identified with global descriptors. The only condition for source and target vertex is that both must be
     * in the local cluster or any of its subclusters. If thats not the case, the function will fail. On success
     * a new GlobalEdge will be created, but not neccessarily a local one. If the vertices are in different cluster
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
    fusion::vector<LocalEdge, GlobalEdge, bool, bool> addEdge(GlobalVertex source, GlobalVertex target) {

        LocalVertex v1,v2;
        LocalEdge e;
        bool d1,d2,d3;
        boost::tie(v1,d1) = getContainingVertex(source);
        boost::tie(v2,d2) = getContainingVertex(target);

        //if one vertex is not accessible from here this function fails
        if(!(d1&&d2)) return fusion::make_vector(LocalEdge(), GlobalEdge(), false, false);

        //if both vertices are in a subcluster this one must do the job as we cant access the local edge from here
        if(v1==v2 && isCluster(v1)) {
            fusion::vector<LocalEdge, GlobalEdge, bool, bool> res = getVertexCluster(v1)->addEdge(source, target);
            fusion::at_c<3>(res)=false;
            return res;
        }

        //check if we already have that Local edge
        boost::tie(e,d3) = boost::edge(v1,v2, *this);
        if(!d3) boost::tie(e,d3) = boost::add_edge(v1, v2, *this);
        if(!d3) return fusion::make_vector(LocalEdge(), GlobalEdge(), false, false);

        //init the bundle corectly for new edge
        GlobalEdge global = { source, target, m_id->generate() };
        edge_bundle_single s;
        fusion::at_c<1>(s) = global;
        fusion::at_c<1>((*this)[e]).push_back(s);

        setChanged();
        return fusion::make_vector(e, global, true, true);

    };

    fusion::vector<LocalEdge, GlobalEdge, bool, bool> addEdgeGlobal(GlobalVertex source, GlobalVertex target) {
        return addEdge(source, target);
    };

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
    std::pair<global_edge_iterator, global_edge_iterator> getGlobalEdges(LocalEdge e) {

        std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[e]);
        global_edge_iterator begin = boost::make_transform_iterator(vec.begin(), global_extractor());
        global_edge_iterator end   = boost::make_transform_iterator(vec.end(), global_extractor());

        setChanged();
        return std::pair<global_edge_iterator, global_edge_iterator>(begin, end);
    };

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
    int getGlobalEdgeCount(LocalEdge e) {

        return fusion::at_c<1>((*this)[e]).size();
    };

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
    std::pair<LocalEdge, bool> getLocalEdge(GlobalEdge e) {
        return getContainingEdge(e);
    };

    /**
     * @brief Get the local edge which holds the specified global one and the subcluster in which it is valid.
     *
     * The function only fails when the global edge is hold by a local one upstream in the cluster
     * herarchy.
     *
     * @param e GlobalEdge for which the containing local one is wanted
     * @return fusion::vector<LocalEdge, ClusterGraph*, bool> with the containing LocalEdge, the cluster which holds it and a bool indicator if function was successful.
     **/
    fusion::vector<LocalEdge, ClusterGraph*, bool> getLocalEdgeGraph(GlobalEdge e) {
        return getContainingEdgeGraph(e);
    };

    /**
     * @brief Get the GlobalVertex assiociated with this local one.
     *
     * @param v LocalVertex
     * @return GlobalVertex
     **/
    GlobalVertex getGlobalVertex(LocalVertex v) const {
        return fusion::at_c<0>((*this)[v]);
    };

    /**
     * @brief Set the GlobalVertex assiociated with this local one.
     *
     * Be carefull, LocalVertices get an global value assigned while created, override it only when your
     * are sure that it is unique
     *
     * @param lv LocalVertex which sould get assigned the global one
     * @param gv The value which the localVertex should get assigned
     * @return GlobalVertex which was assigned
     **/
    GlobalVertex setGlobalVertex(LocalVertex lv, GlobalVertex gv) {
        fusion::at_c<0>((*this)[lv]) = gv;
        return gv;
    };

    /**
     * @brief Get the LocalVertex which corresponds to the golab one
     *
     * The GlobalVertex has to be in this cluster or any of it's subclusters. If its in a subcluster, the returned
     * LocalVertex will represent this cluster. If the GlobalVertex is not in this clusters scope the function fails.
     *
     * @param GlobalVertex
     * @return std::pair< LocalVertex, bool > The LocalVertex containing the global one and an success indicator
     **/
    std::pair<LocalVertex,bool> getLocalVertex(GlobalVertex e) {
        return getContainingVertex(e);
    };

    /**
     * @brief Get the local vertex which holds the specified global one and the subcluster in which it is valid.
     *
     * The function only fails when the global vertex is hold by a local one upstream in the cluster
     * herarchy.
     *
     * @param v GlobalVertex for which the containing local one is wanted
     * @return fusion::vector<LocalVertex, ClusterGraph*, bool> with the containing LocalVertex, the cluster which holds it and a bool indicator if function was successful.
     **/
    fusion::vector<LocalVertex, boost::shared_ptr<ClusterGraph>, bool> getLocalVertexGraph(GlobalVertex v) {
        return getContainingVertexGraph(v);
    };


    /* *******************************************************
     * Remove Handling
     * *******************************************************/
private:

    template<typename Functor>
    struct apply_remove_prediacte {
        Functor& func;
        GlobalVertex vert;
        GlobalEdge edge;
        bool isEdge;

        apply_remove_prediacte(Functor& f, GlobalVertex v) : func(f), vert(v), isEdge(false) {};
        apply_remove_prediacte(Functor& f, GlobalEdge e) : func(f), edge(e), vert(0), isEdge(true) {};
        bool operator()(edge_bundle_single& e) {
            bool res;
            if(isEdge)
                res = (edge==fusion::at_c<1>(e));
            else
                res = (vert==fusion::at_c<1>(e).source) || (vert==fusion::at_c<1>(e).target);

            if(res || vert<0)
                func(fusion::at_c<1>(e));

            return res || vert<0;
        }
    };

    struct placehoder {
        template<typename T>
        void operator()(T t) {};
    };

    template<typename Functor>
    void downstreamRemoveVertex(GlobalVertex v, Functor& f) {

        std::pair<LocalVertex, bool> res = getContainingVertex(v);
        if(!res.second)
            return; //TODO:throw

        //iterate over every edge that connects to the global vertex or the cluster in which it is in
        std::vector<LocalEdge> re; //remove edges
        std::pair<local_out_edge_iterator,  local_out_edge_iterator> it = boost::out_edges(res.first, *this);
        for(; it.first != it.second; it.first++) {
            std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[*(it.first)]);
            vec.erase(std::remove_if(vec.begin(), vec.end(), apply_remove_prediacte<Functor>(f,v)), vec.end());
            if(vec.empty())
                re.push_back(*(it.first));
        };

        std::for_each(re.begin(), re.end(), boost::bind(&ClusterGraph::simpleRemoveEdge, this, _1));

        //if we have the real vertex here and not only a containing cluster we can delete it
        if(!isCluster(res.first)) {
            boost::clear_vertex(res.first, *this); //just to make sure, should be done already
            boost::remove_vertex(res.first, *this);
        };

        //lets go downstream
        for(cluster_iterator it = m_clusters.begin(); it != m_clusters.end(); it++)
            ((*it).second)->downstreamRemoveVertex(v, f);
    };

    void simpleRemoveEdge(LocalEdge e) {
        boost::remove_edge(e, *this);
    };
    

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
    **/
    template<typename Functor>
    void removeVertex(LocalVertex id, Functor& f) {
        removeVertex(getGlobalVertex(id), f);
    };
    //no default template arguments for template functions allowed before c++0x, so a little workaround
    void removeVertex(LocalVertex id) {
        placehoder p;
        removeVertex(getGlobalVertex(id), p);
    };

    /**
    * @brief Removes a vertex from the cluster or it's subclusters and applys functor to removed edges
    *
    * Removes the vertex from the graph or subclusters and invalidates the global vertex id. Also all edges connecting
    * to this vertex will be removed (upstream and downstream) after the functor was applied to them. The functor
    * needs to implement operato()(LocalEdge edge).
    *
    * @param id Global Vertex which should be removed from the graph
    **/
    template<typename Functor>
    void removeVertex(GlobalVertex id, Functor& f) {
        root()->downstreamRemoveVertex(id, f);
    };
    //no default template arguments for template functions allowed before c++0x, so a little workaround
    void removeVertex(GlobalVertex id) {
        placehoder p;
        removeVertex(id, p);
    };

    /**
    * @brief Removes a global Edge from the cluster or it's subclusters
    *
    * Removes the edge from the graph or subclusters and invalidates the global edge id. If the local edge holds
    * only this global one it will be removed also.
    *
    * @param id Global Edge which should be removed from the graph
    * @return bool indicates if the global id could be removed
    **/
    void removeEdge(GlobalEdge id) {
        fusion::vector<LocalEdge, ClusterGraph*, bool> res = getContainingEdgeGraph(id);
        if(!fusion::at_c<2>(res))
            return; //TODO:throw

        placehoder p;
        std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*fusion::at_c<1>(res))[fusion::at_c<0>(res)]);
        vec.erase(std::remove_if(vec.begin(), vec.end(), apply_remove_prediacte<placehoder>(p,id)), vec.end());

        if(vec.empty())
            boost::remove_edge(fusion::at_c<0>(res), *fusion::at_c<1>(res));
    };

    /**
    * @brief Removes a local edge from the cluster and calls the functor for all removed global edges
    *
    * Removes the edge from the graph and invalidates the global edges. The Functor needs to provide
    * operator()(GlobalEdge). If no functor is needed just use boost::remove_edge.
    *
    * @param id Global Edge which should be removed from the graph
    * @return bool indicates if the global id could be removed
    **/
    template<typename Functor>
    void removeEdge(LocalEdge id, Functor& f) {

        std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[id]);
        std::for_each(vec.begin(), vec.end(), boost::bind<void>(boost::ref(apply_remove_prediacte<placehoder>(f,-1)),_1));
        boost::remove_edge(id, *this);
    };


    /* *******************************************************
     * Object Handling
     * *******************************************************/

protected:
    //types needed to distinguish when objects need to be reset
    struct get : public boost::false_type {};
    struct set : public boost::true_type {};

    template<typename Type, typename Obj, typename key>
    struct obj_helper {

        typedef typename object_extractor<Obj>::result_type result_type;

        obj_helper(key k) : m_key(k) {};

        //used with vertex bundle type
        template<typename bundle>
        typename boost::enable_if<boost::is_same<bundle, typename boost::vertex_bundle_type<Graph>::type>,
        result_type >::type operator()(bundle& p) {

            if(Type::value) fusion::for_each(fusion::at_c<2>(p), details::clear_ptr());
            return object_extractor<Obj>()(p);
        }

        //used with edge bundle type and global edge descriptor
        template<typename bundle>
        typename boost::enable_if<mpl::and_<boost::is_same<bundle, typename boost::edge_bundle_type<Graph>::type>,
        boost::is_same<key, GlobalEdge> >, result_type>::type operator()(bundle& p) {

            edge_single_iterator e;
            //need to search the edge_bundle for the global descriptor
            std::vector<edge_bundle_single>& ebsv = fusion::at_c<1>(p);
            for(edge_single_iterator it= ebsv.begin(); it != ebsv.end(); it++) {
                if(global_extractor()(*it) == m_key) {
                    if(Type::value) fusion::for_each(fusion::at_c<0>(*it), details::clear_ptr());
                    e = it;
                    break;
                }
            }
            return object_extractor<Obj>()(*e);
        }

        //used with edge bundle type and local edge descriptor
        template<typename bundle>
        typename boost::enable_if<mpl::and_<boost::is_same<bundle, typename boost::edge_bundle_type<Graph>::type>,
        boost::is_same<key, LocalEdge> >, result_type>::type operator()(bundle& p) {
            if(Type::value) fusion::for_each(fusion::at_c<0>(fusion::at_c<1>(p).front()), details::clear_ptr());
            return object_extractor<Obj>()(fusion::at_c<1>(p).front());
        }

        key m_key;
    };

    template<typename Functor>
    struct valid_ptr_apply {

        Functor& func;
        valid_ptr_apply(Functor& f) : func(f) {};

        template<typename Ptr>
        void operator()(Ptr& p) const {
            if(p)
                func(p);
        }
    };

public:

    /**
    * @brief Get the desired object at the specified vertex or edge
    *
    * This function allows to access the objects stored in the graph. If no object of the desired type
    * was set before, a empty shared_ptr will be returned. Accessing the object at a local edge is a special
    * case, as it can hold many global edges, each with it's own objetcs. Using a LocalEdge as key will
    * always return the object for the first GlobalEdge.
    *
    * @param local or global Vertex/Edge descriptor for which the object is desired
    * @return shared_ptr< Obj > the pointer to the desired object
    **/
    template<typename Obj, typename key>
    boost::shared_ptr<Obj> getObject(key k) {
        return apply_to_bundle(k, obj_helper<get, Obj, key>(k));
    };

    /**
     * @brief Set a object at the specified vertex or edge
     *
     * Sets the given value at the given key. Note that every entity can hold only one object, so setting
     * a new value resets all other objects which were set before. Setting the object at a local edge is a special
     * case, as it can hold many global edges, each with it's own objects. Using a LocalEdge as key will
     * always set the object for the first GlobalEdge.
     *
     * @param k local or global Vertex/Edge descriptor for which the object should be set
     * @param val the object which should be stored
     * @return void
     **/
    template<typename Obj, typename key>
    void setObject(key k, boost::shared_ptr<Obj> val) {
        apply_to_bundle(k, obj_helper<set, Obj, key>(k)) = val;

        setChanged();
    };

    /**
     * @brief Get iterator range for all GlobalEdge objects hold by this local edge
     *
     * LocalEdge's can hold multiple global ones and the iterators can be used to access a specific object type in
     * all global edges hold by this local edge.
     *
     * @param k the LocalEdge over which all Objects should be iterated.
     * @return pair< begin, end > the iterator rang from begin (first element) to end (first undefined element)
     **/
    template<typename Obj>
    std::pair< object_iterator<Obj>, object_iterator<Obj> > getObjects(LocalEdge k) {

        std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[k]);
        object_iterator<Obj> begin(vec.begin(), object_extractor<Obj>());
        object_iterator<Obj> end(vec.end(), object_extractor<Obj>());
        return std::pair< object_iterator<Obj>, object_iterator<Obj> >(begin, end);
    };

    /**
     * @brief Applys the functor to each occurence of an object
     *
     * Each valid object of the given type is extractet and passed to the function object. Vertices
     * and edges are searched for valid object pointers, it happens in this order. When a recursive
     * search is specified, all subclusters are searched too, but the cluster is passt to the Functor
     * first. So make sure a function overload for clusters exist in this case.
     *
     * @param f the functor to which all valid objects get passed to.
     * @param recursive specifies if the subclusters should be searched for objects too
     **/
    template<typename Obj, typename Functor>
    void for_each(Functor& f, bool recursive = false) {

        std::pair<local_vertex_iterator, local_vertex_iterator>  it = boost::vertices(*this);
        for(; it.first != it.second; it.first++) {
            boost::shared_ptr<Obj> ptr =  getObject<Obj>(*(it.first)) ;
            if(ptr)
                f(ptr);
        }

        std::pair<local_edge_iterator, local_edge_iterator> eit = boost::edges(*this);
        for(; eit.first != eit.second; eit.first++) {
            std::pair< object_iterator< Obj >, object_iterator< Obj > > goit =  getObjects<Obj>(*(eit.first));
            for(; goit.first != goit.second; goit.first++) {
                if(*goit.first)
                    f(*goit.first);
            }
        }

        if(recursive) {
            cluster_iterator cit;
            for(cit=m_clusters.begin(); cit != m_clusters.end(); cit++) {
                f((*cit).second);
                (*cit).second->template for_each<Obj>(f, recursive);
            }
        }
    };

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
    void for_each_object(Functor& f, bool recursive = false) {

        valid_ptr_apply<Functor> func(f);

        std::pair<local_vertex_iterator, local_vertex_iterator>  it = boost::vertices(*this);
        for(; it.first != it.second; it.first++) {
            typename details::sps<objects>::type& seq = fusion::at_c<2>((*this)[*it.first]);
            fusion::for_each(seq, func);
        }

        typedef typename std::vector<edge_bundle_single>::iterator iter;
        std::pair<local_edge_iterator, local_edge_iterator> eit = boost::edges(*this);
        for(; eit.first != eit.second; eit.first++) {
            std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[*eit.first]);
            for(iter git = vec.begin(); git != vec.end(); git++) {
                typename details::sps<objects>::type& seq = fusion::at_c<0>(*git);
                fusion::for_each(seq, func);
            }
        }

        if(recursive) {
            cluster_iterator cit;
            for(cit=m_clusters.begin(); cit != m_clusters.end(); cit++) {
                f((*cit).second);
                (*cit).second->for_each_object(f, recursive);
            }
        }
    };

    /* *******************************************************
     * Property Handling
     * *******************************************************/

protected:

    template<typename prop, typename key>
    struct get_prop_helper {

        get_prop_helper(key k) : m_key(k) {};

        typedef typename prop::type base_type;
        typedef base_type& result_type;
        typedef typename mpl::find<vertex_properties, prop>::type vertex_iterator;
        typedef typename mpl::find<edge_properties, prop>::type edge_iterator;
        typedef typename mpl::if_<boost::is_same<vertex_iterator,typename mpl::end<vertex_properties>::type >,
                edge_iterator, vertex_iterator>::type iterator;
        BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<edge_properties>::type > >));

        //used with vertex bundle type
        template<typename bundle>
        typename boost::enable_if<boost::is_same<bundle, typename boost::vertex_bundle_type<Graph>::type>,
        result_type>::type operator()(bundle& p) {
            return property_extractor<prop>()(p);
        }

        //used with edge bundle type
        template<typename bundle>
        typename boost::enable_if<boost::is_same<bundle, typename boost::edge_bundle_type<Graph>::type>,
        result_type>::type operator()(bundle& p) {
            return property_extractor<prop>()(p);
        }

        key m_key;
    };

public:
    /**
    * @brief Get the desired property at the specified vertex or edge
    *
    * This function allows to access the properties stored in the graph. If no property of the desired type
    * was set before, a default construced will be returned. Accessing the property at a global edge will return
    * the property of the holding local edge.
    *
    * @param local or global Vertex/Edge descriptor for which the property is desired
    * @return property::type& the reference to the desired property
    **/
    template<typename property, typename key>
    typename property::type& getProperty(key k) {
        return apply_to_bundle(k, get_prop_helper<property, key>(k));
    };

    /**
     * @brief Set a property at the specified vertex or edge
     *
     * Sets the given value at the given key. Note that every entity can hold one of each property, as opposed
     * to objects. Setting the property at a local edge is a special case, as it can hold many global edges,
     * each with it's own propertys. Using a LocalEdge as key will always set the property for the first GlobalEdge.
     *
     * @param k local or global Vertex/Edge descriptor for which the property should be set
     * @param val the property value which should be stored
     * @return void
     **/
    template<typename property, typename key>
    void setProperty(key k, typename property::type val) {
        apply_to_bundle(k, get_prop_helper<property, key>(k)) = val;

        setChanged();
    };
    
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
    void initIndexMaps() {
      
	//just iterate over all edges and vertices and give them all a unique index
        std::pair<local_vertex_iterator, local_vertex_iterator>  vit = boost::vertices(*this);
        for(int c=0; vit.first != vit.second; vit.first++, c++)
            setProperty<vertex_index_prop>(*vit.first, c);
	
	std::pair<local_edge_iterator, local_edge_iterator>  eit = boost::edges(*this);
        for(int c=0; eit.first != eit.second; eit.first++, c++)
            setProperty<edge_index_prop>(*eit.first, c);
    };



    /********************************************************
    * Vertex and Cluster moving
    * *******************************************************/

    /**
     * @brief Move a vertex to a subcluster
     *
     * Overloaded convinience function which fetches the local descriptor for the cluster reference and calls
     * the full parameter equivalent. Both cluster and vertex must be in the local cluster.
     *
     * @param v the LocalVertex to be moved
     * @param cg reference to the subcluster to which v should be moved
     * @return LocalVertex the local descriptor of the moved vertex in the subcluster
     **/
    LocalVertex moveToSubcluster(LocalVertex v, boost::shared_ptr<ClusterGraph> cg) {

        LocalVertex cv = getClusterVertex(cg);
        return moveToSubcluster(v, cv, cg);
    };

    /**
     * @brief Move a vertex to a subcluster
     *
     * Overloaded convinience function which fetches the the cluster reference for the local descriptor and calls
     * the full parameter equivalent. Both cluster and vertex must be in the local cluster.
     *
     * @param v the LocalVertex to be moved
     * @param Cluster the local vertex descriptor representing the subcluster to which v should be moved
     * @return LocalVertex the local descriptor of the moved vertex in the subcluster
     **/
    LocalVertex moveToSubcluster(LocalVertex v, LocalVertex Cluster) {

        boost::shared_ptr<ClusterGraph> cg = getVertexCluster(Cluster);
        return moveToSubcluster(v, Cluster, cg);
    };

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
    LocalVertex moveToSubcluster(LocalVertex v, LocalVertex Cluster, boost::shared_ptr<ClusterGraph> cg) {

        std::pair<local_out_edge_iterator, local_out_edge_iterator> it =  boost::out_edges(v, *this);

        /* add the later removed edges to the coressponding existing edges
         * (or create new edges between adjacent vertices of moved vertex and cluster).
         * also get the edge between cluster and vertex while iterating */
        for(; it.first!=it.second; it.first++) {

            LocalVertex target = boost::target(*it.first, *this);
            if(target != Cluster) {

                //get or create the edge between the old edge target and the cluster
                LocalEdge e;
                bool done;
                boost::tie(e,done) = boost::edge(target, Cluster, *this);
                if(!done) boost::tie(e,done) = boost::add_edge(target, Cluster, *this);
                //if(!done) TODO: throw

                std::vector<edge_bundle_single>& ep = fusion::at_c<1>((*this)[*it.first]);
                std::vector<edge_bundle_single>& nep = fusion::at_c<1>((*this)[e]);
                nep.insert(nep.end(), ep.begin(), ep.end());
            }
        }

        /* Create new Vertex in Cluster and map the edge to vertices and clusters in the cluster
        * if a connection existed */
        LocalVertex nv= boost::add_vertex((*this)[v], *cg);
        //resort cluster parentship if needed
        if(isCluster(v)) {

            cg->m_clusters[nv] = m_clusters[v];
            cg->m_clusters[nv]->m_parent = cg;
            m_clusters.erase(v);
        }

        std::pair<LocalEdge, bool> moveedge = boost::edge(v, Cluster, *this);
        if(moveedge.second) {
            std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*this)[moveedge.first]);
            for(edge_single_iterator i = vec.begin(); i != vec.end(); i++) {

                //get the global vertex to which the global edge points and find the local vertex holding this
                //global one
                GlobalEdge global = global_extractor()(*i);
                GlobalVertex target;
                //bit cumbersome to support moving clusters
                target = (cg->getContainingVertex(global.source).first == nv) ? global.target : global.source;
                std::pair<LocalVertex, bool> res = cg->getContainingVertex(target);
                //if(!res.second) TODO: throw

                //get or create the edge between the new vertex and the target
                LocalEdge e;
                bool done;
                boost::tie(e,done) = boost::edge(nv, res.first, *cg);
                if(!done) boost::tie(e,done) = boost::add_edge(nv, res.first, *cg);
                //if(!done) TODO: throw

                //push the global edge to the local edge
                fusion::at_c<1>((*cg)[e]).push_back(*i);
            };
        }

        //all global edges concerning the move vertex are processed and it is moved to the subcluster,
        //lets destroy it in the local cluster
        boost::clear_vertex(v, *this);
        boost::remove_vertex(v, *this);

        setChanged();
        cg->setChanged();

        return nv;
    };


    /**
     * @brief Move a vertex to the parent cluster.
     *
     * This function moves a vertex one step up in the subcluster hierarchie and reconnects all other vertices and clusters.
     * The moved vertex will hold it's global descriptor but get a new local one assigned (the one returned). The same
     * stands for all edges which use the moved vertex: global descriptors stay the same, but they are moved to new
     * local edges. Note that this function is the inverse of moveToSubcluster, and doing Pseudocode:
     * moveToParent(moveToSubcluster(v)) does nothing (only the local descriptor of the moved vertex is
     * diffrent afterwards).
     *
     * @param v Local vertex which should be moved to the parents cluster
     * @return LocalVertex the local descriptor of the moved vertex, valid in the parent cluster only.
     **/
    LocalVertex moveToParent(LocalVertex v) {

        //if(isRoot()) TODO:throw

        //create new vertex
        vertex_bundle& vb = (*this)[v];
        LocalVertex nv = boost::add_vertex(vb, *parent());
        //regrouping if needed
        if(isCluster(v)) {
            parent()->m_clusters[nv] = m_clusters[v];
            parent()->m_clusters[nv]->m_parent = m_parent;
            m_clusters.erase(v);
        }

        GlobalVertex gv= fusion::at_c<0>(vb);

        //get all out_edges of this cluster in the parentcluster (because only they can hold relevant global_Edgs)
        std::vector<LocalEdge> edge_vec;
        LocalVertex this_v = parent()->getClusterVertex(sp_base::shared_from_this());
        std::pair<local_out_edge_iterator, local_out_edge_iterator> it = boost::out_edges(this_v, *parent());
        for(; it.first != it.second; it.first++) {
            //iterate all global edges and find relevant ones
            std::vector<edge_bundle_single>& vec = fusion::at_c<1>((*parent())[*it.first]);
            edge_single_iterator i = vec.begin();
            while(i != vec.end()) {

                GlobalEdge global = global_extractor()(*i);
                GlobalVertex target;
                //a bit cumbersome to allow cluster moving
                if(parent()->getContainingVertex(global.source).first == nv) target = global.target;
                else if(parent()->getContainingVertex(global.target).first == nv) target = global.source;
                else {
                    i++;
                    continue;
                }

                std::pair<LocalVertex, bool> res = parent()->getContainingVertex(target);

                //get or create the edge between the new vertex and the target
                LocalEdge e;
                bool done;
                boost::tie(e,done) = boost::edge(nv, res.first, *parent());
                if(!done) boost::tie(e,done) = boost::add_edge(nv, res.first, *parent());
                //if(!done) TODO: throw

                //push the global edge bundle to the new local edge and erase it in the old
                fusion::at_c<1>((*parent())[e]).push_back(*i);
                i = vec.erase(i);
            }
            //see if we should destroy this edge (no global edges remain in local one)
            if(vec.empty()) edge_vec.push_back(*it.first);
        }

        //create a edge between new vertex and this cluster and add all global edges from within this cluster
        it = boost::out_edges(v, *this);
        LocalEdge e;
        if(it.first != it.second) e = boost::add_edge(nv, this_v, *parent()).first;
        for(; it.first != it.second; it.first++) {
            std::vector<edge_bundle_single>& ep = fusion::at_c<1>((*this)[*it.first]);
            std::vector<edge_bundle_single>& nep = fusion::at_c<1>((*parent())[e]);
            nep.insert(nep.end(), ep.begin(), ep.end());
        }

        //all global edges concerning the move vertex are processed and it is moved to the parent,
        //lets destroy it in the local cluster
        boost::clear_vertex(v, *this);
        boost::remove_vertex(v, *this);

        //it's possible that some local edges in the parent are empty now, let's destroy them
        for(std::vector<LocalEdge>::iterator it=edge_vec.begin(); it!=edge_vec.end(); it++)
            boost::remove_edge(*it, *parent());

        setChanged();
        parent()->setChanged();
        return nv;
    };


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
     * one which holds the global vertex. If not successfull the local vertex returned will be
     * invalid and the bool parameter will be false. If recursive = true, all subclusters will
     * be seached too, however, if found there the retourned local vertex will be the vertex
     * representing the toplevel cluster holding the global vertex in the initial graph.
     * */
    std::pair<LocalVertex, bool> getContainingVertex(GlobalVertex id, bool recursive = true) {

        //check all vertices if they are the id
        std::pair<local_vertex_iterator, local_vertex_iterator>  it = boost::vertices(*this);
        for(; it.first != it.second; it.first++) {
            if(id == fusion::at_c<0>((*this)[*it.first]))
                return std::make_pair(*it.first, true);
        }

        //check all clusters if they have the id
        if(recursive) {
            for(cluster_iterator it = m_clusters.begin(); it != m_clusters.end(); it++) {
                std::pair<LocalVertex, bool> res = ((*it).second)->getContainingVertex(id);
                if(res.second) return std::make_pair((*it).first, true);
            }
        }

        return std::make_pair((LocalVertex)NULL, false);
    };

    /* Searches the local vertex holding the specified global one in this and all it's subclusters.
     * If found, the holding local vertex and the graph in which it is valid will be returned.
     * */
    fusion::vector<LocalVertex, boost::shared_ptr<ClusterGraph>, bool> getContainingVertexGraph(GlobalVertex id) {

        LocalVertex v;
        bool done;
        boost::tie(v, done) = getContainingVertex(id);
        if(!done) return fusion::make_vector(LocalVertex(), boost::shared_ptr<ClusterGraph>(), false);

        if(isCluster(v) && (getGlobalVertex(v) != id))
            return m_clusters[v]->getContainingVertexGraph(id);
        else return fusion::make_vector(v,sp_base::shared_from_this(),true);
    };
    
    /* Searches the global edge in all local edges of this graph, and returns the local
     * one which holds the global edge. If not successfull the local edge returned will be
     * invalid and the bool parameter will be false.
     * */
    std::pair<LocalEdge, bool> getContainingEdge(GlobalEdge id) {

        LocalVertex v1,v2;
        bool d1,d2;
        boost::tie(v1,d1) = getContainingVertex(id.source, true);
        boost::tie(v2,d2) = getContainingVertex(id.target, true);

        if(!((d1&&d2) && (v1!=v2)))   return std::make_pair(LocalEdge(), false);

        return boost::edge(v1,v2,*this);
    };

    /* Searches the local edge holding the specified global one in this and all it's subclusters.
     * If found, the holding local edge and the graph in which it is valid will be returned.
     * */
    fusion::vector<LocalEdge, ClusterGraph*, bool> getContainingEdgeGraph(GlobalEdge id) {

        LocalVertex v1,v2;
        bool d1,d2;
        boost::tie(v1,d1) = getContainingVertex(id.source, true);
        boost::tie(v2,d2) = getContainingVertex(id.target, true);

        if(!(d1&&d2))  return fusion::make_vector(LocalEdge(), (ClusterGraph*)NULL, false);
        if(v1==v2)   return m_clusters[v1]->getContainingEdgeGraph(id);

        return fusion::make_vector(boost::edge(v1,v2,*this).first, this, true);
    };

    template<typename functor>
    typename functor::result_type apply_to_bundle(LocalVertex k, functor f) {
        return f((*this)[k]);
    };

    template<typename functor>
    typename functor::result_type apply_to_bundle(LocalEdge k, functor f) {
        return f((*this)[k]);
    };

    template<typename functor>
    typename functor::result_type apply_to_bundle(GlobalVertex k, functor f) {

        //check all vertices if they are the id
        std::pair<local_vertex_iterator, local_vertex_iterator>  it = boost::vertices(*this);
        for(; it.first != it.second; it.first++) {
            vertex_bundle& p = (*this)[*it.first];
            if(k == fusion::at_c<0>(p))
                return f(p);
        }

        //check all clusters if they have the object
        fusion::vector<LocalVertex, boost::shared_ptr<ClusterGraph>, bool> res = getContainingVertexGraph(k);
        if(!fusion::at_c<2>(res)) {
            //TODO: Throw (propeties return reference, but cant init a reference temporarily)
        }

        return fusion::at_c<1>(res)->template apply_to_bundle<functor>(k, f);
    };

    template<typename functor>
    typename functor::result_type apply_to_bundle(GlobalEdge k, functor f) {

        LocalVertex v1,v2;
        bool d1,d2;
        boost::tie(v1,d1) = getContainingVertex(k.source);
        boost::tie(v2,d2) = getContainingVertex(k.target);

        if(!(d1&&d2)) {
            //TODO:Throw
        }

        if((v1==v2) && isCluster(v1)) return m_clusters[v1]->apply_to_bundle(k, f);
        else {
            LocalEdge e;
            bool done;
            boost::tie(e, done) = boost::edge(v1,v2,*this);
            //if(!done) TODO: throw, as there has to be a edge!
            return f((*this)[e]);
        };


    };

public:
	//may hold cluster properties which have Eigen3 objects and therefore need alignment
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

} //namespace solver

#endif // CLUSTERGRAPH_HPP




