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

#ifndef DCM_PARSER_H
#define DCM_PARSER_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include <iosfwd>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "opendcm/core/clustergraph.hpp"

#include "property_parser.hpp"
#include "object_parser.hpp"
#include "edge_vertex_parser.hpp"
#include "extractor.hpp"

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

struct sp : qi::grammar<IIterator, std::string()> {

    qi::rule<IIterator, std::string()> start;
    sp() : sp::base_type(start) {
    start %= +qi::char_;
};
static void print(std::string s) {
    std::cout<<"parsed string:"<<std::endl<<s<<std::endl<<"done print string"<<std::endl;
};

};

template<typename Sys>
struct parser : qi::grammar<IIterator, Sys(), qi::space_type> {

    typedef typename Sys::Cluster graph;

    parser();

    qi::rule<IIterator, Sys(), qi::space_type> system;
    details::kernel_prop_par<Sys> kernel_prop;
    details::system_prop_par<Sys> system_prop;

    qi::rule<IIterator, boost::shared_ptr<graph>(Sys*), qi::locals<int, std::vector<boost::shared_ptr<graph> > >, qi::space_type> cluster;
    details::cluster_prop_par<Sys> cluster_prop;

    details::obj_par<Sys> objects;

    details::vertex_parser<Sys> vertex;
    details::edge_parser<Sys> edge;

    sp str;
    Injector<Sys> in;
};

}

#ifndef DCM_EXTERNAL_STATE
#include "imp/parser_imp.hpp"
#endif

#endif //DCM_PARSER_H
