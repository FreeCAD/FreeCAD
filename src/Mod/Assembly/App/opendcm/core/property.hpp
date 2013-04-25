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

#ifndef GCM_PROPERTY_H
#define GCM_PROPERTY_H

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/buffer_concepts.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container/vector.hpp>

#include <boost/mpl/find.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/property_map/property_map.hpp>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

struct vertex_property {};
struct edge_property {};
struct cluster_property {};
struct object_property {};

namespace details {

template<typename Graph>
struct vertex_selector {
    typedef typename boost::graph_traits<Graph>::vertex_descriptor key_type;
    typedef typename Graph::vertex_properties sequence_type;
};

template<typename Graph>
struct edge_selector {
    typedef typename boost::graph_traits<Graph>::edge_descriptor key_type;
    typedef typename Graph::edge_properties sequence_type;
};

template< typename Kind, typename Graph>
struct property_selector : public mpl::if_<boost::is_same<Kind, vertex_property>,
        vertex_selector<Graph>, edge_selector<Graph> >::type {};

template<typename T>
struct property_type {
    typedef typename T::type type;
};
template<typename T>
struct property_kind {
    typedef typename T::kind type;
};

//property vector to a fusion sequence of the propety types
template<typename T>
struct pts { //property type sequence
    typedef typename mpl::transform<T, details::property_type<mpl::_1> >::type ptv;
    typedef typename fusion::result_of::as_vector< ptv >::type type;
};
}

template<typename T>
struct is_edge_property : boost::is_same<typename T::kind,edge_property> {};

template<typename T>
struct is_vertex_property : boost::is_same<typename T::kind,vertex_property> {};

template<typename T>
struct is_cluster_property : boost::is_same<typename T::kind,cluster_property> {};

template<typename T>
struct is_object_property : boost::is_same<typename T::kind,object_property> {};

template <typename Property, typename Graph>
class property_map  {

public:
    typedef typename dcm::details::property_selector<typename Property::kind, Graph>::key_type key_type;
    typedef typename Property::type value_type;
    typedef typename Property::type&  reference;
    typedef boost::lvalue_property_map_tag category;

    typedef Property property;
    typedef typename dcm::details::property_selector<typename Property::kind, Graph>::sequence_type sequence;

    property_map(Graph& g)
        : m_graph(g) { }

    Graph& m_graph;
};

template<typename P, typename G>
typename property_map<P,G>::value_type	get(const property_map<P,G>& map,
        typename property_map<P,G>::key_type key)  {

    typedef property_map<P,G> map_t;
    typedef typename mpl::find<typename map_t::sequence, typename map_t::property>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<typename map_t::sequence>::type, iterator>::type distance;
    return  fusion::at<distance>(fusion::at_c<0>(map.m_graph[key]));
};

template <typename P, typename G>
void  put(const property_map<P,G>& map,
          typename property_map<P,G>::key_type key,
          const typename property_map<P,G>::value_type& value)  {

    typedef property_map<P,G> map_t;
    typedef typename mpl::find<typename map_t::sequence, typename map_t::property>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<typename map_t::sequence>::type, iterator>::type distance;
    fusion::at<distance>(fusion::at_c<0>(map.m_graph[key])) = value;
};


template <typename P, typename G>
typename property_map<P,G>::reference at(const property_map<P,G>& map,
        typename property_map<P,G>::key_type key) {
    typedef property_map<P,G> map_t;
    typedef typename mpl::find<typename map_t::sequence, typename map_t::property>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<typename map_t::sequence>::type, iterator>::type distance;
    return fusion::at<distance>(fusion::at_c<0>(map.m_graph[key]));
}


//now create some standart properties
//***********************************

struct empty_prop {
  typedef int kind;
  typedef int type;
};

struct type_prop {
    //states the type of a cluster
    typedef cluster_property kind;
    typedef int type;
};

struct changed_prop {
    typedef cluster_property kind;
    typedef bool type;
};

template<typename T>
struct id_prop {
    typedef object_property kind;
    typedef T type;
};

}


#endif //GCM_PROPERTY_H
