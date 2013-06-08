#ifndef DCM_OBJECT_GENERATOR_IMP_H
#define DCM_OBJECT_GENERATOR_IMP_H


#include "traits_impl.hpp"
#include "object_generator.hpp"
#include "property_generator_imp.hpp"

#include "boost/phoenix/fusion/at.hpp"

using namespace boost::spirit::karma;
namespace karma = boost::spirit::karma;
namespace phx = boost::phoenix;
namespace fusion = boost::fusion;

namespace dcm {

typedef std::ostream_iterator<char> Iterator;


namespace details {

template<typename Sys, typename Object, typename Gen>
obj_grammar<Sys, Object,Gen>::obj_grammar() : obj_grammar<Sys, Object,Gen>::base_type(start) {
    Gen::init(subrule);
    start = lit("\n<Object>") << '+' << eol << subrule
            << prop[phx::bind(&obj_grammar::getProperties, _val, karma::_1)]
            << '-' << eol << lit("</Object>");
};

template<typename Sys, typename Object, typename Gen>
void obj_grammar<Sys, Object,Gen>::getProperties(boost::shared_ptr<Object> ptr, typename details::pts<typename Object::Sequence>::type& seq) {

    if(ptr) seq = ptr->m_properties;
    else {
        //TODO: throw
    };
};

template<typename Sys>
obj_gen<Sys>::obj_gen() : obj_gen<Sys>::base_type(obj) {

    obj =    -(eps(valid<0>::value) << eps(phx::at_c<index<0>::value>(_val)) << fusion::at<index<0> >(rules)[karma::_1 = phx::at_c<index<0>::value>(_val)])
             << -(eps(valid<1>::value) << eps(phx::at_c<index<1>::value>(_val)) << fusion::at<index<1> >(rules)[karma::_1 = phx::at_c<index<1>::value>(_val)])
             << -(eps(valid<2>::value) << eps(phx::at_c<index<2>::value>(_val)) << fusion::at<index<2> >(rules)[karma::_1 = phx::at_c<index<2>::value>(_val)])
             << -(eps(valid<3>::value) << eps(phx::at_c<index<3>::value>(_val)) << fusion::at<index<3> >(rules)[karma::_1 = phx::at_c<index<3>::value>(_val)])
             << -(eps(valid<4>::value) << eps(phx::at_c<index<4>::value>(_val)) << fusion::at<index<4> >(rules)[karma::_1 = phx::at_c<index<4>::value>(_val)])
             << -(eps(valid<5>::value) << eps(phx::at_c<index<5>::value>(_val)) << fusion::at<index<5> >(rules)[karma::_1 = phx::at_c<index<5>::value>(_val)])
             << -(eps(valid<6>::value) << eps(phx::at_c<index<6>::value>(_val)) << fusion::at<index<6> >(rules)[karma::_1 = phx::at_c<index<6>::value>(_val)])
             << -(eps(valid<7>::value) << eps(phx::at_c<index<7>::value>(_val)) << fusion::at<index<7> >(rules)[karma::_1 = phx::at_c<index<7>::value>(_val)])
             << -(eps(valid<8>::value) << eps(phx::at_c<index<8>::value>(_val)) << fusion::at<index<8> >(rules)[karma::_1 = phx::at_c<index<8>::value>(_val)])
             << -(eps(valid<9>::value) << eps(phx::at_c<index<9>::value>(_val)) << fusion::at<index<9> >(rules)[karma::_1 = phx::at_c<index<9>::value>(_val)]);

};

} //namespace details
}//dcm

#endif //DCM_OBJECT_GENERATOR_H
