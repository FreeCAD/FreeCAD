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

#ifndef DCM_DEFINES_STATE_H
#define DCM_DEFINES_STATE_H

#include "opendcm/core/property.hpp"
#include "opendcm/core/clustergraph.hpp"
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


namespace dcm {
namespace details {

struct cluster_vertex_prop {
    typedef GlobalVertex type;
    typedef cluster_property kind;
};
}
}

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (T1)(T2)(T3)(T4),
    (dcm::ClusterGraph) (T1)(T2)(T3)(T4),
    (int, test)
    (typename dcm::details::pts<T3>::type, m_properties))

//fusion adopt struct needs to avoid commas for type definition, as this breaks the macro syntax. 
//we use this ugly nested struct to declare the dcm system from template parameters without commas
template<typename T1>
struct t1 {
    template<typename T2>
    struct t2 {
        template<typename T3>
        struct t3 {
            template<typename T4>
            struct t4 {
	      typedef dcm::System<T1,T2,T3,T4> type;
            };
        };
    };
};

BOOST_FUSION_ADAPT_TPL_STRUCT(
    (Kernel)(M1)(M2)(M3),
    (dcm::System)(Kernel)(M1)(M2)(M3),
    (typename t1<Kernel>::template t2<M1>::template t3<M2>::template t4<M3>::type::OptionOwner::Properties, m_options->m_properties)
    (typename Kernel::Properties, m_kernel.m_properties)
    (boost::shared_ptr<typename t1<Kernel>::template t2<M1>::template t3<M2>::template t4<M3>::type::Cluster>, m_cluster)
)

#endif
