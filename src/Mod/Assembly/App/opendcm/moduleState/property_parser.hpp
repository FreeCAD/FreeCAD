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

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

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

template<typename PropList, typename Prop, typename Par>
struct prop_parser : qi::grammar<IIterator, qi::unused_type(typename details::pts<PropList>::type*), qi::space_type> {

    typename Par::parser subrule;
    qi::rule<IIterator, qi::unused_type(typename details::pts<PropList>::type*), qi::space_type> start;
    prop_parser();
    prop_parser(const prop_parser& other) : prop_parser::base_type(start) {};
};

template<typename Sys, typename seq, typename state>
struct prop_parser_fold : mpl::fold< seq, state,
        mpl::if_< dcm::parser_parse<mpl::_2, Sys>,
        mpl::push_back<mpl::_1, prop_parser<seq, mpl::_2, dcm::parser_parser<mpl::_2, Sys, IIterator> > >,
        mpl::_1 > > {};

//grammar for a fusion sequence of properties.
template<typename Sys, typename PropertyList>
struct prop_par : qi::grammar<IIterator, typename details::pts<PropertyList>::type(), qi::space_type> {

    //create a vector with the appropriate rules for all needed properties.
    typedef typename prop_parser_fold<Sys, PropertyList, mpl::vector<> >::type sub_rules_sequence;
    //the type of the propertylist rule
    typedef qi::rule<IIterator, qi::unused_type(typename details::pts<PropertyList>::type*), qi::space_type> parent_rule;
    //we need to store all recursive created rules
    typedef typename mpl::fold< sub_rules_sequence, mpl::vector0<>,
    mpl::push_back<mpl::_1, parent_rule> >::type parent_rules_sequence;

    typename fusion::result_of::as_vector<sub_rules_sequence>::type sub_rules;
    typename fusion::result_of::as_vector<parent_rules_sequence>::type parent_rules;

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

template<typename Sys>
struct kernel_prop_par : public prop_par<Sys, typename Sys::Kernel::PropertySequence> {
    kernel_prop_par();
};

template<typename Sys>
struct system_prop_par : public prop_par<Sys, typename Sys::OptionOwner::PropertySequence> {
    system_prop_par();
};

} //DCM
} //details

#ifndef DCM_EXTERNAL_STATE
#include "imp/property_parser_imp.hpp"
#endif

#endif
