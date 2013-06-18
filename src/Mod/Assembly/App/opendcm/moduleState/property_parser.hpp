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

#ifndef DCM_PROPERTY_PARSER_H
#define DCM_PROPERTY_PARSER_H

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <boost/mpl/less.hpp>
#include <boost/mpl/int.hpp>

namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;
namespace mpl = boost::mpl;

namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

namespace details {

struct empty_parser : public qi::grammar<IIterator> {
    qi::rule<IIterator> start;
    empty_parser(): empty_parser::base_type(start) {
	start = qi::eps(true);
    };
    empty_parser(const empty_parser& other) : empty_parser::base_type(start) {};
};

template<typename Prop>
struct skip_parser : public qi::grammar<IIterator, typename Prop::type()> {
    qi::rule<IIterator, typename Prop::type()> start;
    skip_parser() : skip_parser<Prop>::base_type(start) {
	start = qi::eps(true);
    };
    skip_parser(const skip_parser& other) : skip_parser::base_type(start) {};
};

template<typename Prop, typename Par>
struct prop_parser : qi::grammar<IIterator, typename Prop::type(), qi::space_type> {

    typename Par::parser subrule;
    qi::rule<IIterator, typename Prop::type(), qi::space_type> start;
    prop_parser();
    prop_parser(const prop_parser& other) : prop_parser::base_type(start) {};
};

template<typename Sys, typename seq, typename state>
struct prop_parser_fold : mpl::fold< seq, state,
        mpl::if_< dcm::parser_parse<mpl::_2, Sys>,
        mpl::push_back<mpl::_1,
        prop_parser<mpl::_2, dcm::parser_parser<mpl::_2, Sys, IIterator> > >,
        mpl::push_back<mpl::_1, skip_parser<mpl::_2> > > > {};

//grammar for a fusion sequence of properties. currently max. 10 properties are supported
template<typename Sys, typename PropertyList>
struct prop_par : qi::grammar<IIterator, typename details::pts<PropertyList>::type(), qi::space_type> {

    //create a vector with the appropriate rules for all properties.
    typedef typename prop_parser_fold<Sys, PropertyList, mpl::vector<> >::type init_rules_sequence;
    //allow max 10 types as the following code expect this
    BOOST_MPL_ASSERT((mpl::less_equal< mpl::size<init_rules_sequence>, mpl::int_<10> >));
    //we want to process 10 elements, so create a vector with (10-prop.size()) empty rules
    //and append it to our rules vector
    typedef mpl::range_c<int,0, mpl::minus< mpl::int_<10>, mpl::size<init_rules_sequence> >::value > range;
    typedef typename mpl::fold< range,
				init_rules_sequence,
				mpl::push_back<mpl::_1, empty_parser> >::type rules_sequence;

    typename fusion::result_of::as_vector<rules_sequence>::type rules;
    qi::rule<IIterator, typename details::pts<PropertyList>::type(), qi::space_type> prop;

    prop_par();
};

    //special prop classes for better externalisaton, therefore the outside constructor to avoid auto inline
    template<typename Sys> 
    struct cluster_prop_par : public prop_par<Sys, typename Sys::Cluster::cluster_properties> {
      cluster_prop_par();
    };
   
    template<typename Sys>     
    struct vertex_prop_par : public prop_par<Sys, typename Sys::Cluster::vertex_properties> {
      vertex_prop_par();
    };
    
    template<typename Sys> 
    struct edge_prop_par : public prop_par<Sys, typename Sys::Cluster::edge_properties> {
      edge_prop_par();
    };

} //DCM
} //details

#ifndef USE_EXTERNAL
  #include "property_parser_imp.hpp"
#endif

#endif
