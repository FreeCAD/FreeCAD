#ifndef DCM_PROPERTY_GENERATOR_H
#define DCM_PROPERTY_GENERATOR_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include <boost/fusion/include/as_vector.hpp>
#include <boost/spirit/include/karma.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/less_equal.hpp>

#include "traits.hpp"

namespace karma = boost::spirit::karma;
namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

namespace dcm {

typedef std::ostream_iterator<char> Iterator;

namespace details {

//a grammar that does nothing returns true
struct empty_grammar : public karma::grammar<Iterator> {
    karma::rule<Iterator> start;
    empty_grammar(): empty_grammar::base_type(start) {
        start = karma::eps(true);
    };
    empty_grammar(const empty_grammar& other) : empty_grammar::base_type(start) {};
};

template<typename Prop>
struct skip_grammar : public karma::grammar<Iterator, typename Prop::type&()> {
    karma::rule<Iterator, typename Prop::type&()> start;
    skip_grammar() : skip_grammar<Prop>::base_type(start) {
	start = karma::eps(true);
    };
    skip_grammar(const skip_grammar& other) : skip_grammar::base_type(start) {};
};

//grammar for a single property
template<typename Prop, typename Gen>
struct prop_grammar : public karma::grammar<Iterator, typename Prop::type&()> {
    typename Gen::generator subrule;
    karma::rule<Iterator, typename Prop::type&()> start;
    prop_grammar();
    prop_grammar(const prop_grammar& other) : prop_grammar::base_type(start) {};
};

template<typename Sys, typename seq, typename state>
struct prop_generator_fold : mpl::fold< seq, state,
        mpl::if_< parser_generate<mpl::_2, Sys>,
        mpl::push_back<mpl::_1,
        prop_grammar<mpl::_2, dcm::parser_generator<mpl::_2, Sys, Iterator> > >,
        mpl::push_back<mpl::_1, skip_grammar<mpl::_2> > > > {};

//grammar for a fusion sequence of properties. currently max. 10 properties are supported
template<typename Sys, typename PropertyList>
struct prop_gen : karma::grammar<Iterator, typename details::pts<PropertyList>::type&()> {

    //create a vector with the appropriate rules for all properties.
    typedef typename prop_generator_fold<Sys, PropertyList, mpl::vector<> >::type init_rules_sequence;
    //allow max 10 types as the following code expect this
    BOOST_MPL_ASSERT((mpl::less_equal< mpl::size<init_rules_sequence>, mpl::int_<10> >));
    //we want to process 10 elements, so create a vector with (10-prop.size()) empty rules
    //and append it to our rules vector
    typedef mpl::range_c<int,0, mpl::minus< mpl::int_<10>, mpl::size<init_rules_sequence> >::value > range;
    typedef typename mpl::fold< range,
				init_rules_sequence,
				mpl::push_back<mpl::_1, empty_grammar> >::type rules_sequence;

    typename fusion::result_of::as_vector<rules_sequence>::type rules;
    karma::rule<Iterator, typename details::pts<PropertyList>::type&()> prop;

    prop_gen();
};

//special prop classes for better externalisaton, therefore the outside constructor to avoid auto inline
template<typename Sys>
struct cluster_prop_gen : public prop_gen<Sys, typename Sys::Cluster::cluster_properties> {
    cluster_prop_gen();
};

template<typename Sys>
struct vertex_prop_gen : public prop_gen<Sys, typename Sys::Cluster::vertex_properties> {
    vertex_prop_gen();
};

template<typename Sys>
struct edge_prop_gen : public prop_gen<Sys, typename Sys::Cluster::edge_properties> {
    edge_prop_gen();
};

template<typename Sys>
struct system_prop_gen : public prop_gen<Sys, typename Sys::OptionOwner::PropertySequence> {
    system_prop_gen();
};

template<typename Sys>
struct kernel_prop_gen : public prop_gen<Sys, typename Sys::Kernel::PropertySequence> {
    kernel_prop_gen();
};

}//details
}//dcm

#ifndef DCM_EXTERNAL_STATE
#include "imp/property_generator_imp.hpp"
#endif

#endif //DCM_PROPERTY_GENERATOR_H
