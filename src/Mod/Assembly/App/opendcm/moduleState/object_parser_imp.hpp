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

#ifndef DCM_OBJECT_PARSER_IMP_H
#define DCM_OBJECT_PARSER_IMP_H

#include "object_parser.hpp"
#include "property_parser_imp.hpp"

namespace dcm {
namespace details {

template<typename Sys, typename Object, typename Par>
obj_parser<Sys, Object, Par>::obj_parser(): obj_parser::base_type(start) {
    Par::init(subrule);
    start = qi::lit("<Object>") >> subrule(qi::_r1)[qi::_val = qi::_1]
            >> qi::eps(qi::_val)[ phx::bind(&Sys::template push_back<Object>, qi::_r1, qi::_val)]
            >> prop[phx::bind(&obj_parser::setProperties, qi::_val, qi::_1)]
            >> qi::lit("</Object>");
};

template<typename Sys, typename Object, typename Par>
void obj_parser<Sys, Object, Par>::setProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::Sequence>::type& seq) {
    if(ptr) ptr->m_properties = seq;
};

template<typename Sys>
obj_par<Sys>::obj_par(): obj_par<Sys>::base_type(obj) {

    obj =      -(qi::eps(valid<0>::value) >> fusion::at<index<0> >(rules)(qi::_r1)[phx::at_c<index<0>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<1>::value) >> fusion::at<index<1> >(rules)(qi::_r1)[phx::at_c<index<1>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<2>::value) >> fusion::at<index<2> >(rules)(qi::_r1)[phx::at_c<index<2>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<3>::value) >> fusion::at<index<3> >(rules)(qi::_r1)[phx::at_c<index<3>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<4>::value) >> fusion::at<index<4> >(rules)(qi::_r1)[phx::at_c<index<4>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<5>::value) >> fusion::at<index<5> >(rules)(qi::_r1)[phx::at_c<index<5>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<6>::value) >> fusion::at<index<6> >(rules)(qi::_r1)[phx::at_c<index<6>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<7>::value) >> fusion::at<index<7> >(rules)(qi::_r1)[phx::at_c<index<7>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<8>::value) >> fusion::at<index<8> >(rules)(qi::_r1)[phx::at_c<index<8>::value>(qi::_val) = qi::_1])
               >> -(qi::eps(valid<9>::value) >> fusion::at<index<9> >(rules)(qi::_r1)[phx::at_c<index<9>::value>(qi::_val) = qi::_1]);

};

}//details
}//DCM


#endif
