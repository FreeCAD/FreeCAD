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

#ifndef DCM_GENERATOR_IMP_H
#define DCM_GENERATOR_IMP_H

#include "generator.hpp"
#include "opendcm/core/clustergraph.hpp"
//#include "karma_trans.hpp"

#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


BOOST_FUSION_ADAPT_TPL_STRUCT(
    (T1)(T2)(T3)(T4),
    (dcm::ClusterGraph) (T1)(T2)(T3)(T4),
    (int, test)
    (typename dcm::details::pts<T3>::type, m_cluster_bundle))


namespace boost { namespace spirit { namespace traits
{
    template <typename T1, typename T2, typename T3, typename T4>
    struct transform_attribute<dcm::ClusterGraph<T1,T2,T3,T4>* const, dcm::ClusterGraph<T1,T2,T3,T4>&, karma::domain>
    {
        typedef dcm::ClusterGraph<T1,T2,T3,T4>& type;
        static type pre(dcm::ClusterGraph<T1,T2,T3,T4>* const& val) { 
	    return *val; 
        }
    };
}}}

namespace dcm {

template<typename Sys>
generator<Sys>::generator() : generator<Sys>::base_type(start) {
           
    cluster %= karma::omit[karma::int_] << cluster_prop  << -vertex_range[phx::bind(&Extractor<Sys>::getVertexRange, ex, karma::_val, karma::_1)]
	       << -karma::buffer["\n" << edge_range[phx::bind(&Extractor<Sys>::getEdgeRange, ex, karma::_val, karma::_1)]]
               << -karma::buffer["\n" << (cluster_pair % karma::eol)[phx::bind(&Extractor<Sys>::getClusterRange, ex, karma::_val, karma::_1)]] << "-\n"
               << "</Cluster>";

    cluster_pair %= karma::lit("<Cluster id=") << karma::int_ <<  ">+" 
		    << karma::attr_cast<graph*,graph&>(cluster); 
		    
    start %= karma::lit("<Cluster id=0>+") << cluster;
};

}//namespace dcm

#endif //DCM_GENERATOR_H



