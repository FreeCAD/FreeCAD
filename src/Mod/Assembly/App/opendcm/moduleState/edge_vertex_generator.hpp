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

#ifndef DCM_EDGE_GENERATOR_H
#define DCM_EDGE_GENERATOR_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include "property_generator.hpp"
#include "object_generator.hpp"
#include "extractor.hpp"

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

namespace karma = boost::spirit::karma;
namespace phx = boost::phoenix;

namespace dcm {
namespace details {
  
template<typename Sys>
struct edge_generator : karma::grammar<Iterator, std::vector<fusion::vector3<typename Sys::Cluster::edge_bundle, GlobalVertex, GlobalVertex> >()> {
  
      edge_generator();

      karma::rule<Iterator, std::vector<fusion::vector3<typename Sys::Cluster::edge_bundle, GlobalVertex, GlobalVertex> >()> edge_range;
      karma::rule<Iterator, fusion::vector3<typename Sys::Cluster::edge_bundle, GlobalVertex, GlobalVertex>()> edge;
      karma::rule<Iterator, std::vector<typename Sys::Cluster::edge_bundle_single>&()> globaledge_range;
      karma::rule<Iterator, typename Sys::Cluster::edge_bundle_single()> globaledge;
      details::edge_prop_gen<Sys> edge_prop;
      details::obj_gen<Sys> objects;	
      Extractor<Sys> ex;
};

template<typename Sys>
struct vertex_generator : karma::grammar<Iterator, std::vector<typename Sys::Cluster::vertex_bundle>()> {
  
      vertex_generator();

      karma::rule<Iterator, std::vector<typename Sys::Cluster::vertex_bundle>()> vertex_range;
      karma::rule<Iterator, typename Sys::Cluster::vertex_bundle()> vertex;
      details::vertex_prop_gen<Sys> vertex_prop;
      details::obj_gen<Sys> objects;
      Extractor<Sys> ex;
};

}//details
}//dcm

#ifndef DCM_EXTERNAL_STATE
  #include "imp/edge_vertex_generator_imp.hpp"
#endif

#endif