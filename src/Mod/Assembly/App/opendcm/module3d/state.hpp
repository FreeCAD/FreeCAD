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

#ifndef DCM_MODULE3D_STATE_HPP
#define DCM_MODULE3D_STATE_HPP

#include "module.hpp"
#include "opendcm/moduleState/traits.hpp"
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace karma = boost::spirit::karma;
namespace ascii = boost::spirit::karma::ascii;
namespace phx = boost::phoenix;

namespace dcm {
  
namespace details {
    
struct geom_visitor : public boost::static_visitor<int> {

    template<typename T>
    int operator()(T& i) const {
        return geometry_traits<T>::tag::weight::value;
    };
}; 

template<typename T>
int getWeight(boost::shared_ptr<T> ptr) {
    return boost::apply_visitor(geom_visitor(), ptr->m_geometry);
};

template<typename Kernel>
void getStdVector(typename Kernel::Vector& eigen, std::vector<typename Kernel::number_type>& vec) {
    vec.resize(eigen.size());
    for(int i=0; i<eigen.size(); i++)
	vec[i] = eigen(i);
};

}
 
template<typename System>
struct parser_generate< typename Module3D::type<System>::Geometry3D, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_generator< typename Module3D::type<System>::Geometry3D, System, iterator > {

    typedef typename Sys::Kernel Kernel;
    typedef typename typename Module3D::type<System>::Geometry3D Geometry;
    typedef karma::rule<iterator, boost::shared_ptr<Geometry>() > generator;
    static void init(generator& r) {
        r = karma::lit("<type>Geometry3D</type>\n<class>")
	    << ascii::string[karma::_1 = phx::bind(&details::getWeight<Geometry>, karma::_val)]
	    << "</class>\n<value>" 
	    << (karma::double_ % " ")[phx::bind(&details::getStdVector<Kernel>, )]
    };
};

template<typename System>
struct parser_parse< typename Module3D::type<System>::Geometry3D, System>
  : public mpl::true_{};

template<typename System, typename iterator>
struct parser_parser< typename Module3D::type<System>::Geometry3D, System, iterator > {

    typedef typename Module3D::type<System>::Geometry3D object_type;
    
    typedef qi::rule<iterator, boost::shared_ptr<object_type>(System*), qi::space_type> parser;
    static void init(parser& r) {
        r = qi::lexeme[qi::lit("<type>object 1 prop</type>")[ qi::_val = 
		  phx::construct<boost::shared_ptr<object_type> >( phx::new_<object_type>(*qi::_r1))]] >> ("<value>HaHAHAHAHA</value>");
    };
};
  
}


#endif //DCM_MODULE3D_STATE_HPP