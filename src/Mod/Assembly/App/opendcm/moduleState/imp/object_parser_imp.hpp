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

#include "../object_parser.hpp"
#include "property_parser_imp.hpp"
#include "boost/phoenix/fusion/at.hpp"

namespace dcm {
namespace details {

template<typename srs, typename prs, typename dist>
typename boost::enable_if<mpl::less< dist, mpl::size<srs> >, void >::type recursive_obj_init(srs& sseq, prs& pseq) {
    
    if(dist::value == 0) {
      fusion::at<dist>(pseq) %= fusion::at<dist>(sseq)(qi::_r1, qi::_r2);
    }
    else {
      fusion::at<dist>(pseq) %= fusion::at<typename mpl::prior< typename mpl::max<dist, mpl::int_<1> >::type >::type>(pseq)(qi::_r1, qi::_r2) | fusion::at<dist>(sseq)(qi::_r1, qi::_r2);
    }

    recursive_obj_init<srs, prs, typename mpl::next<dist>::type>(sseq, pseq);
};

template<typename srs, typename prs, typename dist>
typename boost::disable_if<mpl::less< dist, mpl::size<srs> >, void >::type recursive_obj_init(srs& sseq, prs& pseq) {};

template<typename Sys, typename ObjList, typename Object, typename Par>
obj_parser<Sys, ObjList, Object, Par>::obj_parser(): obj_parser::base_type(start) {

    typedef typename mpl::find<ObjList, Object>::type::pos pos;

    Par::init(subrule);
    start = qi::lit("<Object>") >> subrule(qi::_r2)[phx::at_c<pos::value>(*qi::_r1) = qi::_1]
            >> qi::eps(phx::at_c<pos::value>(*qi::_r1))[ phx::bind(&Sys::template push_back<Object>, qi::_r2, phx::at_c<pos::value>(*qi::_r1))]
            >> prop[phx::bind(&obj_parser::setProperties, phx::at_c<pos::value>(*qi::_r1), qi::_1)]
            >> qi::lit("</Object>"); 
};

template<typename Sys, typename ObjList, typename Object, typename Par>
void obj_parser<Sys, ObjList, Object, Par>::setProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::PropertySequence>::type& seq) {
    if(ptr) ptr->m_properties = seq;
};

template<typename ParentRuleSequence, typename Rule>
typename boost::disable_if<typename fusion::result_of::empty<ParentRuleSequence>::type, void>::type
initalizeLastObjRule(ParentRuleSequence& pr, Rule& r) {
    r = *(fusion::back(pr)(&qi::_val, qi::_r1));
};

template<typename ParentRuleSequence, typename Rule>
typename boost::enable_if<typename fusion::result_of::empty<ParentRuleSequence>::type, void>::type
initalizeLastObjRule(ParentRuleSequence& pr, Rule& r) {};


template<typename Sys>
obj_par<Sys>::obj_par(): obj_par<Sys>::base_type(obj) {

    recursive_obj_init<typename fusion::result_of::as_vector<sub_rules_sequence>::type,
                       typename fusion::result_of::as_vector<parent_rules_sequence>::type,
                       mpl::int_<0> >(sub_rules, parent_rules);

    initalizeLastObjRule(parent_rules, obj);
};

}//details
}//DCM


#endif
