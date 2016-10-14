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

#ifndef DCM_OBJECT_PARSER_H
#define DCM_OBJECT_PARSER_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include "property_parser.hpp"

namespace dcm {
namespace details {

//grammar for a single object
template<typename Sys, typename ObjList, typename Object, typename Par>
struct obj_parser : public qi::grammar<IIterator, qi::unused_type(typename details::sps<ObjList>::type*, Sys*), qi::space_type> {
    typename Par::parser subrule;
    qi::rule<IIterator, qi::unused_type(typename details::sps<ObjList>::type*, Sys*), qi::space_type> start;
    prop_par<Sys, typename Object::PropertySequence > prop;

    obj_parser();

    static void setProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::PropertySequence>::type& seq);
};

//when objects should not be generated we need to get a empy rule, as obj_rule_init
//trys always to access the rules attribute and when the parser_generator trait is not
//specialitzed it's impossible to have the attribute type right in the unspecialized trait
template<typename Sys, typename seq, typename state>
struct obj_parser_fold : mpl::fold< seq, state,
        mpl::if_< parser_parse<mpl::_2, Sys>,
        mpl::push_back<mpl::_1,
        obj_parser<Sys, seq, mpl::_2, dcm::parser_parser<mpl::_2, Sys, IIterator> > >,
        mpl::_1 > > {};

//currently max. 10 objects are supported
template<typename Sys>
struct obj_par : public qi::grammar<IIterator,
typename details::sps<typename Sys::objects>::type(Sys*),
         qi::space_type> {

             typedef typename Sys::objects ObjectList;
             //create a vector with the appropriate rules for all needed objects.
             typedef typename obj_parser_fold<Sys, ObjectList, mpl::vector<> >::type sub_rules_sequence;
             //the type of the objectlist rule
             typedef qi::rule<IIterator, qi::unused_type(typename details::sps<ObjectList>::type*, Sys*), qi::space_type> parent_rule;
             //we need to store all recursive created rules
             typedef typename mpl::fold< sub_rules_sequence, mpl::vector0<>,
             mpl::push_back<mpl::_1, parent_rule> >::type parent_rules_sequence;

             typename fusion::result_of::as_vector<sub_rules_sequence>::type sub_rules;
             typename fusion::result_of::as_vector<parent_rules_sequence>::type parent_rules;

             qi::rule<IIterator, typename details::sps<ObjectList>::type(Sys*), qi::space_type> obj;

             obj_par();
         };

}//details
}//DCM

#ifndef DCM_EXTERNAL_STATE
#include "imp/object_parser_imp.hpp"
#endif

#endif
