#ifndef DCM_OBJECT_GENERATOR_H
#define DCM_OBJECT_GENERATOR_H

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3
#endif

#include "property_generator.hpp"

namespace fusion = boost::fusion;

namespace dcm {
  
typedef std::ostream_iterator<char> Iterator;


namespace details {
  
    //grammar for a single object
    template<typename Sys, typename Object, typename Gen>
    struct obj_grammar : public karma::grammar<Iterator, boost::shared_ptr<Object>()> {
        typename Gen::generator subrule;
        karma::rule<Iterator, boost::shared_ptr<Object>()> start;
        details::prop_gen<Sys, typename Object::PropertySequence > prop;

        obj_grammar();
        static void getProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::PropertySequence>::type& seq);
    };

    //when objects should not be generated we need to get a empy rule, as obj_rule_init
    //trys always to access the rules attribute and when the parser_generator trait is not
    //specialitzed it's impossible to have the attribute type right in the unspecialized trait
    template<typename Sys, typename seq, typename state>
    struct obj_generator_fold : mpl::fold< seq, state,
            mpl::if_< parser_generate<mpl::_2, Sys>,
            mpl::push_back<mpl::_1,
            obj_grammar<Sys, mpl::_2, dcm::parser_generator<mpl::_2, Sys, Iterator> > >,
            mpl::push_back<mpl::_1, details::empty_grammar > > > {};

    //currently max. 10 objects are supported
    template<typename Sys>
    struct obj_gen : public karma::grammar<Iterator, typename details::sps<typename Sys::objects>::type()> {

	typedef typename Sys::objects ObjectList;
      
        //create a vector with the appropriate rules for all objects. Do this with the rule init struct, as it gives
        //automatic initialisation of the rules when the objects are created
        typedef typename obj_generator_fold<Sys, ObjectList,mpl::vector<> >::type init_rules_vector;
        //push back a empty rule so that we know where to go when nothing is to do
        typedef typename mpl::push_back<init_rules_vector, empty_grammar >::type rules_vector;

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
        karma::rule<Iterator, typename details::sps<ObjectList>::type()> obj;

        obj_gen();
    };
} //namespace details
}//dcm

#ifndef DCM_EXTERNAL_STATE
  #include "imp/object_generator_imp.hpp"
#endif

#endif //DCM_OBJECT_GENERATOR_H