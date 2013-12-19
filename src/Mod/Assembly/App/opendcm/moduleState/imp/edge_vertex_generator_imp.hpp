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

#ifndef DCM_EDGE_GENERATOR_IMP_H
#define DCM_EDGE_GENERATOR_IMP_H

#include "../edge_vertex_generator.hpp"
#include "boost/phoenix/fusion/at.hpp"

namespace dcm {
namespace details {
  
template<typename Sys>
edge_generator<Sys>::edge_generator() : edge_generator<Sys>::base_type(edge_range) {
  
        globaledge = karma::int_[phx::bind(&Extractor<Sys>::getGlobalEdgeID, &ex, karma::_val, karma::_1)]
                     << " source=" << karma::int_[phx::bind(&Extractor<Sys>::getGlobalEdgeSource, &ex, karma::_val, karma::_1)]
                     << " target=" << karma::int_[phx::bind(&Extractor<Sys>::getGlobalEdgeTarget, &ex, karma::_val, karma::_1)] << '>'
                     << "#" << objects[karma::_1 = phx::at_c<0>(karma::_val)] << "$\n" ;


        globaledge_range = *(karma::lit("<GlobalEdge id=")<<globaledge<<karma::lit("</GlobalEdge>"));

        edge =  karma::lit("source=")<<karma::int_[karma::_1 = phx::at_c<1>(karma::_val)] << " target="<<karma::int_[karma::_1 = phx::at_c<2>(karma::_val)] << ">#"
                << edge_prop[karma::_1 = phx::at_c<0>(phx::at_c<0>(karma::_val))]
                << karma::eol << globaledge_range[karma::_1 = phx::at_c<1>(phx::at_c<0>(karma::_val))] << '$' << karma::eol;

        edge_range = (karma::lit("<Edge ") << edge << karma::lit("</Edge>")) % karma::eol;
};

template<typename Sys>
vertex_generator<Sys>::vertex_generator() : vertex_generator<Sys>::base_type(vertex_range) {
  
        vertex = karma::int_ << ">#" << vertex_prop << objects << "$\n";

        vertex_range = '\n' << (karma::lit("<Vertex id=") << vertex  << karma::lit("</Vertex>")) % karma::eol;
};

}//details
}//dcm

#endif