#ifndef DCM_PROPERTY_GENERATOR_IMP_H
#define DCM_PROPERTY_GENERATOR_IMP_H

#include "../property_generator.hpp"
#include "traits_impl.hpp"

namespace dcm {

typedef std::ostream_iterator<char> Iterator;

namespace details {

//grammar for a single property
template<typename Prop, typename Gen>
prop_grammar<Prop, Gen>::prop_grammar() : prop_grammar<Prop, Gen>::base_type(start) {
  
    Gen::init(subrule);
    start =  karma::lit("\n<Property>") << '#' << karma::eol << subrule
             << '$' << karma::eol << karma::lit("</Property>");
};

template<typename Sys, typename PropertyList>
prop_gen<Sys, PropertyList>::prop_gen() : prop_gen<Sys, PropertyList>::base_type(prop) {
  
    prop  =    fusion::at_c<0>(rules) << fusion::at_c<1>(rules) << fusion::at_c<2>(rules)
	    << fusion::at_c<3>(rules) << fusion::at_c<4>(rules) << fusion::at_c<5>(rules)
	    << fusion::at_c<6>(rules) << fusion::at_c<7>(rules) << fusion::at_c<8>(rules)
	    << fusion::at_c<9>(rules);
};

template<typename Sys>
cluster_prop_gen<Sys>::cluster_prop_gen() : prop_gen<Sys, typename Sys::Cluster::cluster_properties>() {};

template<typename Sys>
vertex_prop_gen<Sys>::vertex_prop_gen() : prop_gen<Sys, typename Sys::Cluster::vertex_properties>() {};

template<typename Sys>
edge_prop_gen<Sys>::edge_prop_gen() : prop_gen<Sys, typename Sys::Cluster::edge_properties>() {};

template<typename Sys>
system_prop_gen<Sys>::system_prop_gen() : prop_gen<Sys, typename Sys::OptionOwner::PropertySequence>() {};

template<typename Sys>
kernel_prop_gen<Sys>::kernel_prop_gen() : prop_gen<Sys, typename Sys::Kernel::PropertySequence>() {};


}//details
}//dcm

#endif //DCM_PROPERTY_GENERATOR_H
