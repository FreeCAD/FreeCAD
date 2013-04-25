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

#ifndef DCM_PROPERTY_PARSER_IMP_H
#define DCM_PROPERTY_PARSER_IMP_H

#include "property_parser.hpp"


namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

namespace details {

template<typename Prop, typename Par>
prop_parser<Prop, Par>::prop_parser() : prop_parser<Prop, Par>::base_type(start) {
    Par::init(subrule);
    start %=  qi::lit("<Property>") >> subrule >> qi::lit("</Property>");
};


template<typename Sys, typename PropertyList>
prop_par<Sys, PropertyList>::prop_par() : prop_par<Sys, PropertyList>::base_type(prop) {

    prop %=  fusion::at_c<0>(rules) >> fusion::at_c<1>(rules) >> fusion::at_c<2>(rules)
	    >> fusion::at_c<3>(rules) >> fusion::at_c<4>(rules) >> fusion::at_c<5>(rules)
	    >> fusion::at_c<6>(rules) >> fusion::at_c<7>(rules) >> fusion::at_c<8>(rules)
	    >> fusion::at_c<9>(rules);
};

template<typename Sys>
cluster_prop_par<Sys>::cluster_prop_par() : prop_par<Sys, typename Sys::Cluster::cluster_properties>() {};

template<typename Sys>
vertex_prop_par<Sys>::vertex_prop_par() : prop_par<Sys, typename Sys::Cluster::vertex_properties>() {};

template<typename Sys>
edge_prop_par<Sys>::edge_prop_par() : prop_par<Sys, typename Sys::Cluster::edge_properties>() {};

} //DCM
} //details

#endif
