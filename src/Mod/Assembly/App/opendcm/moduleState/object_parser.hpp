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

#include "property_parser.hpp"

namespace dcm {
namespace details {

template<typename Obj, typename Sys>
struct empty_obj_parser : public qi::grammar<IIterator, boost::shared_ptr<Obj>(Sys*), qi::space_type> {
    qi::rule<IIterator, boost::shared_ptr<Obj>(Sys*), qi::space_type> start;
    empty_obj_parser(): empty_obj_parser::base_type(start) {
    //start = qi::eps(false);
};
};

//grammar for a single object
template<typename Sys, typename Object, typename Par>
struct obj_parser : public qi::grammar<IIterator, boost::shared_ptr<Object>(Sys*), qi::space_type> {
    typename Par::parser subrule;
    qi::rule<IIterator, boost::shared_ptr<Object>(Sys*), qi::space_type> start;
    prop_par<Sys, typename Object::Sequence > prop;

    obj_parser();

    static void setProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::Sequence>::type& seq);
};

//when objects should not be generated we need to get a empy rule, as obj_rule_init
//trys always to access the rules attribute and when the parser_generator trait is not
//specialitzed it's impossible to have the attribute type right in the unspecialized trait
template<typename Sys, typename seq, typename state>
struct obj_parser_fold : mpl::fold< seq, state,
        mpl::if_< parser_parse<mpl::_2, Sys>,
        mpl::push_back<mpl::_1,
        obj_parser<Sys, mpl::_2, dcm::parser_parser<mpl::_2, Sys, IIterator> > >,
        mpl::push_back<mpl::_1, empty_obj_parser<mpl::_2, Sys> > > > {};

//currently max. 10 objects are supported
template<typename Sys>
struct obj_par : public qi::grammar<IIterator,
typename details::sps<typename Sys::objects>::type(Sys*),
         qi::space_type> {

             typedef typename Sys::objects ObjectList;

             //create a vector with the appropriate rules for all objects. Do this with the rule init struct, as it gives
             //automatic initialisation of the rules when the objects are created
             typedef typename obj_parser_fold<Sys, ObjectList, mpl::vector<> >::type init_rules_vector;
             //push back a empty rule so that we know where to go when nothing is to do
             typedef typename mpl::push_back<init_rules_vector,
             empty_obj_parser<typename mpl::back<ObjectList>::type, Sys> >::type rules_vector;

             //create the fusion sequence of our rules
             typedef typename fusion::result_of::as_vector<rules_vector>::type rules_sequnce;

             //this struct returns the right accessvalue for the sequences. If we access a value bigger than the property vector size
             //we use the last rule, as we made sure this is an empty one
             template<int I>
             struct index : public mpl::if_< mpl::less<mpl::int_<I>, mpl::size<ObjectList> >,
             mpl::int_<I>, typename mpl::size<ObjectList>::prior >::type {};
             //this struct tells us if we should execute the generator
             template<int I>
             struct valid : public mpl::less< mpl::int_<I>, mpl::size<ObjectList> > {};

             rules_sequnce rules;
             qi::rule<IIterator, typename details::sps<ObjectList>::type(Sys*), qi::space_type> obj;

             obj_par();
         };

}//details
}//DCM

#ifndef USE_EXTERNAL
  #include "property_parser_imp.hpp"
#endif

#endif
