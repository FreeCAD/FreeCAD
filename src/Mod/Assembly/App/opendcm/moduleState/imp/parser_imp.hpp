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

#include "../parser.hpp"
#include "../defines.hpp"

namespace boost {
namespace spirit {
namespace traits
{
template <typename T1, typename T2, typename T3, typename T4>
struct transform_attribute<boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> >, typename dcm::ClusterGraph<T1,T2,T3,T4>::Properties, qi::domain>
{
    typedef typename dcm::ClusterGraph<T1,T2,T3,T4>::Properties& type;
    static type pre(boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> >& val) {
        return val->m_properties;
    }
    static void post(boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> >const& val, typename dcm::ClusterGraph<T1,T2,T3,T4>::Properties const& attr) {}
    static void fail(boost::shared_ptr<dcm::ClusterGraph<T1,T2,T3,T4> > const&) {}
};
}
}
}

namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

template<typename Sys>
parser<Sys>::parser() : parser<Sys>::base_type(system) {

    cluster %= qi::lit("<Cluster id=") >> qi::omit[qi::int_[qi::_a = qi::_1]] >> ">"
               >> -(qi::eps(qi::_a > 0)[qi::_val = phx::construct<boost::shared_ptr<graph> >(phx::new_<typename Sys::Cluster>())])
               >> qi::eps[phx::bind(&Sys::Cluster::setCopyMode, &(*qi::_val), true)]
               >> qi::eps[phx::bind(&Injector<Sys>::setVertexProperty, &in, &(*qi::_val), qi::_a)]
               >> qi::attr_cast<boost::shared_ptr<graph>, typename graph::Properties>(cluster_prop >> qi::eps)
               >> qi::omit[(*cluster(qi::_r1))[qi::_b = qi::_1]]
               >> qi::omit[*vertex(&(*qi::_val), qi::_r1)]
               >> qi::omit[*edge(&(*qi::_val), qi::_r1)]
               >> qi::eps[phx::bind(&Injector<Sys>::addClusters, &in, qi::_b, &(*qi::_val))]
               >> qi::eps[phx::bind(&Sys::Cluster::setCopyMode, &(*qi::_val), false)]
               >> "</Cluster>";

    system %= qi::lit("<openDCM>") >> -system_prop
              >> qi::lit("<Kernel>") >> -kernel_prop >> qi::lit("</Kernel>")
              >> cluster(&qi::_val) >> qi::lit("</openDCM>");
};

}
#endif //DCM_PARSER_H
