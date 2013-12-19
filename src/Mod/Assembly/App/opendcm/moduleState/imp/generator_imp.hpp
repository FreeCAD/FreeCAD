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

#include "../generator.hpp"
#include "opendcm/core/clustergraph.hpp"
#include "../defines.hpp"

#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/struct.hpp>

namespace boost {
namespace spirit {
namespace traits
{
template <typename T1, typename T2, typename T3, typename T4>
struct transform_attribute<boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> > const, dcm::ClusterGraph<T1,T2,T3,T4>&, karma::domain>
{
    typedef dcm::ClusterGraph<T1,T2,T3,T4>& type;
    static type pre(boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> > const& val) {
        return *val;
    }
};
}
}
}

namespace dcm {

template<typename Sys>
generator<Sys>::generator() : generator<Sys>::base_type(start) {

    cluster %= karma::omit[karma::int_] << cluster_prop
               << -karma::buffer[karma::eol << (cluster_pair % karma::eol)[phx::bind(&Extractor<Sys>::getClusterRange, &ex, karma::_val, karma::_1)]]
               << -vertex_range[phx::bind(&Extractor<Sys>::getVertexRange, &ex, karma::_val, karma::_1)]
               << -karma::buffer[karma::eol << edge_range[phx::bind(&Extractor<Sys>::getEdgeRange, &ex, karma::_val, karma::_1)]]
               << "$" << karma::eol
               << karma::lit("</Cluster>");

    cluster_pair %= karma::lit("<Cluster id=") << karma::int_ <<  ">#"
                    << qi::attr_cast<boost::shared_ptr<graph>, graph&>(cluster);

    system %= system_prop << karma::lit("<Kernel>#") << kernel_prop
              << "$" << karma::eol << karma::lit("</Kernel>") << karma::eol
	      << karma::lit("<Cluster id=0>#") << qi::attr_cast<boost::shared_ptr<graph>, graph&>(cluster);

    start %= karma::lit("<openDCM>#") << karma::eol << system << "$" << karma::eol << karma::lit("</openDCM>");
};

}//namespace dcm

#endif //DCM_GENERATOR_H



