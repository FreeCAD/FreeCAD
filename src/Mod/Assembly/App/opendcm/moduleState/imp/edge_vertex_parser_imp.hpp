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

#ifndef DCM_EDGE_PARSER_IMP_H
#define DCM_EDGE_PARSER_IMP_H

#include "../edge_vertex_parser.hpp"
#include "boost/phoenix/fusion/at.hpp"

namespace dcm {
namespace details {
	
template<typename Sys>
edge_parser<Sys>::edge_parser() : edge_parser<Sys>::base_type(edge) {

    global_edge = qi::lit("<GlobalEdge") >> qi::lit("id=") >> qi::int_[phx::bind(&GlobalEdge::ID, phx::at_c<1>(qi::_val)) = qi::_1]
                  >> qi::lit("source=") >> qi::int_[phx::bind(&GlobalEdge::source, phx::at_c<1>(qi::_val)) = qi::_1]
                  >> qi::lit("target=") >> qi::int_[phx::bind(&GlobalEdge::target, phx::at_c<1>(qi::_val)) = qi::_1] >> '>'
                  >> objects(qi::_r1)[phx::at_c<0>(qi::_val) = qi::_1] >> "</GlobalEdge>";

    edge   = (qi::lit("<Edge") >> "source=" >> qi::int_ >> "target=" >> qi::int_ >> '>')[qi::_val = phx::bind((&Sys::Cluster::addEdgeGlobal), qi::_r1, qi::_1, qi::_2)]
             >> edge_prop[phx::bind(&Injector<Sys>::setEdgeProperties, &in, qi::_r1, phx::at_c<0>(qi::_val), qi::_1)]
             >> (*global_edge(qi::_r2))[phx::bind(&Injector<Sys>::setEdgeBundles, &in, qi::_r1, phx::at_c<0>(qi::_val), qi::_1)]
             >> ("</Edge>");
};

template<typename Sys>
vertex_parser<Sys>::vertex_parser() : vertex_parser<Sys>::base_type(vertex) {
	
    vertex = qi::lit("<Vertex id=") >> qi::int_[qi::_val = phx::bind(&Sys::Cluster::addVertex, qi::_r1, qi::_1)]
             >> '>' >> prop[phx::bind(&Injector<Sys>::setVertexProperties, &in, qi::_r1, phx::at_c<0>(qi::_val), qi::_1)]
             >> objects(qi::_r2)[phx::bind(&Injector<Sys>::setVertexObjects, &in, qi::_r1, phx::at_c<0>(qi::_val), qi::_1)]
             >> ("</Vertex>");
};

}//details
}//dcm

#endif
