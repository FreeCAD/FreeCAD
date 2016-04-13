/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef DCM_EXTRACTOR_H
#define DCM_EXTRACTOR_H

#include "defines.hpp"
#include <opendcm/core/clustergraph.hpp>
#include <boost/fusion/include/at_c.hpp>

namespace fusion = boost::fusion;

namespace dcm {

typedef std::ostream_iterator<char> Iterator;

template<typename Sys>
struct Extractor {

    typedef typename boost::graph_traits<typename Sys::Cluster>::vertex_iterator viter;
    typedef typename boost::graph_traits<typename Sys::Cluster>::edge_iterator eiter;

    void getVertexRange(typename Sys::Cluster& cluster, std::vector<typename Sys::Cluster::vertex_bundle>& range) {
        std::pair<viter, viter> res = boost::vertices(cluster);
        for(; res.first != res.second; res.first++)
            range.push_back(cluster[*res.first]);

    };
    void getEdgeRange(typename Sys::Cluster& cluster,
                      std::vector<fusion::vector3<typename Sys::Cluster::edge_bundle, GlobalVertex, GlobalVertex> >& range) {

        std::pair<eiter, eiter> res = boost::edges(cluster);
        for(; res.first != res.second; res.first++)
            range.push_back(fusion::make_vector(cluster[*res.first],
                                                cluster.getGlobalVertex(boost::source(*res.first, cluster)),
                                                cluster.getGlobalVertex(boost::target(*res.first, cluster))));

    };
    void getGlobalEdgeSource(typename Sys::Cluster::edge_bundle_single b, int& source) {
        source = fusion::at_c<1>(b).source;
    };
    void getGlobalEdgeTarget(typename Sys::Cluster::edge_bundle_single b, int& target) {
        target = fusion::at_c<1>(b).target;
    };
    void getGlobalEdgeID(typename Sys::Cluster::edge_bundle_single b, int& id) {
        id = fusion::at_c<1>(b).ID;
    };
    void setVertexID(typename Sys::Cluster* cluster, LocalVertex v, long& l) {
        if(v)
            l = cluster->getGlobalVertex(v);
        else
            l = 0;
    };
    void getClusterRange(typename Sys::Cluster& cluster, std::vector<std::pair<GlobalVertex, boost::shared_ptr<typename Sys::Cluster> > >& range) {

        typedef typename Sys::Cluster::const_cluster_iterator iter;

        for(iter it = cluster.m_clusters.begin(); it != cluster.m_clusters.end(); it++) {
            range.push_back(std::make_pair(cluster.getGlobalVertex((*it).first), (*it).second));
        };
    };
};

template<typename Sys>
struct Injector {

    void setClusterProperties(typename Sys::Cluster* cluster,
                              typename details::pts<typename Sys::Cluster::cluster_properties>::type& prop) {
        cluster->m_properties = prop;
    };
    void setVertexProperties(typename Sys::Cluster* cluster, LocalVertex v,
                             typename details::pts<typename Sys::vertex_properties>::type& prop) {
        fusion::at_c<1>(cluster->operator[](v)) = prop;
    };
    void setVertexObjects(typename Sys::Cluster* cluster, LocalVertex v,
                          typename details::sps<typename Sys::objects>::type& obj) {
        fusion::at_c<2>(cluster->operator[](v)) = obj;
    };

    void setEdgeProperties(typename Sys::Cluster* cluster, LocalEdge e,
                           typename details::pts<typename Sys::edge_properties>::type& prop) {
        fusion::at_c<0>(cluster->operator[](e)) = prop;
    };
    void setEdgeBundles(typename Sys::Cluster* cluster, LocalEdge e,
                        std::vector<typename Sys::Cluster::edge_bundle_single>& bundles) {
        fusion::at_c<1>(cluster->operator[](e)) = bundles;
    };
    void setVertexProperty(typename Sys::Cluster* cluster, int value) {
        cluster->template setProperty<details::cluster_vertex_prop>(value);
    };
    void addClusters(std::vector<boost::shared_ptr<typename Sys::Cluster> >& clusters, typename Sys::Cluster* cluster) {

        //vertices for the cluster need to be added already (as edges need vertices created)
        //so we don't create a vertex here.
        typename std::vector<boost::shared_ptr<typename Sys::Cluster> >::iterator it;
        for(it = clusters.begin(); it != clusters.end(); it++) {
            LocalVertex v = cluster->getLocalVertex((*it)->template getProperty<details::cluster_vertex_prop>()).first;
            cluster->m_clusters[v] = *it;
        };
    };
};



}//namespace dcm

#endif //DCM_GENERATOR_H




