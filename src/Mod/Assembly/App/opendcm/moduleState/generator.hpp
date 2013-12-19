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

#ifndef DCM_GENERATOR_H
#define DCM_GENERATOR_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include "property_generator.hpp"
#include "edge_vertex_generator.hpp"
#include "extractor.hpp"

#include <opendcm/core/clustergraph.hpp>

#include "traits.hpp"
#include "imp/traits_impl.hpp"
#include "indent.hpp"

#include <boost/mpl/int.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace karma = boost::spirit::karma;
namespace phx = boost::phoenix;
namespace fusion = boost::fusion;

namespace dcm {

typedef std::ostream_iterator<char> Iterator;

template<typename Sys>
struct generator : karma::grammar<Iterator, Sys&()> {

    typedef typename Sys::Cluster graph;
    typedef typename graph::Properties graph_bundle;
    typedef typename boost::graph_traits<graph>::vertex_iterator viter;
    typedef typename boost::graph_traits<graph>::edge_iterator eiter;

    generator();

    karma::rule<Iterator, Sys& ()> start;

    karma::rule<Iterator, std::pair<GlobalVertex, boost::shared_ptr<graph> >()> cluster_pair;
    karma::rule<Iterator, graph&()> cluster;   
    karma::rule<Iterator, Sys&()>   system;
    
    details::cluster_prop_gen<Sys> cluster_prop;
    details::system_prop_gen<Sys>  system_prop;
    details::kernel_prop_gen<Sys>  kernel_prop;

    details::vertex_generator<Sys> vertex_range;
    details::edge_generator<Sys> edge_range;

    Extractor<Sys> ex;
};

}//namespace dcm

#ifndef DCM_EXTERNAL_STATE
#include "imp/generator_imp.hpp"
#endif

#endif //DCM_GENERATOR_H



