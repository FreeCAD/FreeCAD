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

#ifndef DCM_PROPERTY_PARSER_IMP_H
#define DCM_PROPERTY_PARSER_IMP_H

#include "../property_parser.hpp"
#include "traits_impl.hpp"

#include <boost/fusion/include/back.hpp>
#include <boost/phoenix/fusion/at.hpp>


namespace dcm {

typedef boost::spirit::istream_iterator IIterator;

namespace details {

template<typename srs, typename prs, typename dist>
typename boost::enable_if<mpl::less< dist, mpl::size<srs> >, void >::type recursive_init(srs& sseq, prs& pseq) {

    if(dist::value == 0) {
        fusion::at<dist>(pseq) %= fusion::at<dist>(sseq)(qi::_r1);
    }
    else {
        fusion::at<dist>(pseq) %= fusion::at<typename mpl::prior< typename mpl::max<dist, mpl::int_<1> >::type >::type>(pseq)(qi::_r1) | fusion::at<dist>(sseq)(qi::_r1);
    }

    recursive_init<srs, prs, typename mpl::next<dist>::type>(sseq, pseq);
};

template<typename srs, typename prs, typename dist>
typename boost::disable_if<mpl::less< dist, mpl::size<srs> >, void >::type recursive_init(srs& sseq, prs& pseq) {};

template<typename ParentRuleSequence, typename Rule>
typename boost::disable_if<typename fusion::result_of::empty<ParentRuleSequence>::type, void>::type
initalizeLastRule(ParentRuleSequence& pr, Rule& r) {
    r = *(fusion::back(pr)(&qi::_val));
};

template<typename ParentRuleSequence, typename Rule>
typename boost::enable_if<typename fusion::result_of::empty<ParentRuleSequence>::type, void>::type
initalizeLastRule(ParentRuleSequence& pr, Rule& r) {};


template<typename PropList, typename Prop, typename Par>
prop_parser<PropList, Prop, Par>::prop_parser() : prop_parser<PropList, Prop, Par>::base_type(start) {

    typedef typename mpl::find<PropList, Prop>::type::pos pos;

    Par::init(subrule);
    //start =  qi::lit("<Property>") >> subrule[phx::at_c<pos::value>(*qi::_r1) = qi::_1] >> qi::lit("</Property>");
};

template<typename Sys, typename PropertyList>
prop_par<Sys, PropertyList>::prop_par() : prop_par<Sys, PropertyList>::base_type(prop) {

    recursive_init<typename fusion::result_of::as_vector<sub_rules_sequence>::type,
                   typename fusion::result_of::as_vector<parent_rules_sequence>::type,
                   mpl::int_<0> >(sub_rules, parent_rules);

    //we need to specialy treat empty sequences
    initalizeLastRule(parent_rules, prop);
};

template<typename Sys>
cluster_prop_par<Sys>::cluster_prop_par() : prop_par<Sys, typename Sys::Cluster::cluster_properties>() {};

template<typename Sys>
vertex_prop_par<Sys>::vertex_prop_par() : prop_par<Sys, typename Sys::Cluster::vertex_properties>() {};

template<typename Sys>
edge_prop_par<Sys>::edge_prop_par() : prop_par<Sys, typename Sys::Cluster::edge_properties>() {};

template<typename Sys>
kernel_prop_par<Sys>::kernel_prop_par() : prop_par<Sys, typename Sys::Kernel::PropertySequence>() {};

template<typename Sys>
system_prop_par<Sys>::system_prop_par() : prop_par<Sys, typename Sys::OptionOwner::PropertySequence>() {};

} //DCM
} //details

#endif
