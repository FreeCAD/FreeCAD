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

#ifndef DCM_PARSER_IMP_H
#define DCM_PARSER_IMP_H

#include <boost/spirit/include/qi_attr_cast.hpp>

#include "opendcm/core/system.hpp"

#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (T1)(T2)(T3)(T4),
    (dcm::ClusterGraph) (T1)(T2)(T3)(T4),
    (typename dcm::details::pts<T3>::type, m_cluster_bundle))

#include "parser.hpp"


namespace boost { namespace spirit { namespace traits
{
    template <typename T1, typename T2, typename T3, typename T4>
    struct transform_attribute<dcm::ClusterGraph<T1,T2,T3,T4>*, dcm::ClusterGraph<T1,T2,T3,T4>, qi::domain>
    {
        typedef dcm::ClusterGraph<T1,T2,T3,T4>& type;
        static type pre(dcm::ClusterGraph<T1,T2,T3,T4>* const& val) { 
	    return *val; 
        }
        static void post(dcm::ClusterGraph<T1,T2,T3,T4>* const& val, dcm::ClusterGraph<T1,T2,T3,T4> const& attr) {}
        static void fail(dcm::ClusterGraph<T1,T2,T3,T4>* const&) {}
    };
}}}

namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

template<typename Sys>
parser<Sys>::parser() : parser<Sys>::base_type(cluster) {

    cluster %= qi::lit("<Cluster id=") >> qi::omit[qi::int_[qi::_a = qi::_1]] >> ">"
	      >> -(qi::eps( qi::_a > 0 )[qi::_val = phx::new_<typename Sys::Cluster>()])
	      >> qi::eps[phx::bind(&Injector<Sys>::setVertexProperty, in, qi::_val, qi::_a)]
              >> qi::attr_cast<graph*, graph>(cluster_prop >> qi::eps)
              >> qi::omit[*vertex(qi::_val, qi::_r1)]
              >> qi::omit[*edge(qi::_val, qi::_r1)]
              >> qi::omit[*(cluster(qi::_r1)[phx::bind(&Injector<Sys>::addCluster, in, qi::_val, qi::_1)])]
              >> "</Cluster>";// >> str[&sp::print];
};

}
#endif //DCM_PARSER_H
