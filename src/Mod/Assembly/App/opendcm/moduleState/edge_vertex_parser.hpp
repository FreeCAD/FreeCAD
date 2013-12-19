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

#ifndef DCM_EDGE_VERTEX_PARSER_H
#define DCM_EDGE_VERTEX_PARSER_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include <boost/spirit/include/qi.hpp>
#include "opendcm/core/clustergraph.hpp"
#include "extractor.hpp"
#include "object_parser.hpp"

namespace qi = boost::spirit::qi;
namespace fusion = boost::fusion;

namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

namespace details {

template<typename Sys>
struct edge_parser : qi::grammar< IIterator, fusion::vector<LocalEdge, GlobalEdge, bool, bool>(typename Sys::Cluster*, Sys*),
       qi::space_type > {

           edge_parser();
           details::obj_par<Sys> objects;
		   Injector<Sys> in;

           qi::rule<IIterator, fusion::vector<LocalEdge, GlobalEdge, bool, bool>(typename Sys::Cluster*, Sys*), qi::space_type> edge;
           qi::rule<IIterator, typename Sys::Cluster::edge_bundle_single(Sys*), qi::space_type> global_edge;
           details::edge_prop_par<Sys> edge_prop;

       };

template<typename Sys>
struct vertex_parser : qi::grammar< IIterator, fusion::vector<LocalVertex, GlobalVertex>(typename Sys::Cluster*, Sys*),
       qi::space_type> {

           vertex_parser();

           details::obj_par<Sys> objects;
		   Injector<Sys> in;

           qi::rule<IIterator, fusion::vector<LocalVertex, GlobalVertex>(typename Sys::Cluster*, Sys*), qi::space_type> vertex;
           details::vertex_prop_par<Sys> prop;

       };

}
}

#ifndef DCM_EXTERNAL_STATE
#include "imp/edge_vertex_parser_imp.hpp"
#endif

#endif
