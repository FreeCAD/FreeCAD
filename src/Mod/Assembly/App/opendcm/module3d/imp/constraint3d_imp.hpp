/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef DCM_CONSTRAINT_3D_IMP_H
#define DCM_CONSTRAINT_3D_IMP_H

#include "../module.hpp"

#ifdef DCM_EXTERNAL_CORE
#include "opendcm/core/imp/constraint_imp.hpp"
#include "opendcm/core/imp/object_imp.hpp"
#include "opendcm/core/imp/clustergraph_imp.hpp"
#endif

// #ifdef DCM_EXTERNAL_CORE
// //following macros are used for externalization. As constraint->initialize is very compiler intensive,
// //especially with module states recursive incarnation, we explicitly initiate all possible calls to this
// //function so that it must only be compiled once, one function at a time. To get all possible equation
// //combinations boost pp is needed.
// 
// #include <boost/preprocessor.hpp>
// #include <boost/preprocessor/for.hpp>
// #include <boost/preprocessor/comparison/less.hpp>
// #include <boost/preprocessor/seq/size.hpp>
// #include <boost/preprocessor/seq/enum.hpp>
// #include <boost/preprocessor/seq/elem.hpp>
// #include <boost/preprocessor/seq/push_front.hpp>
// #include <boost/preprocessor/seq/remove.hpp>

/*
// 
// #define SEQUENCE (dcm::Distance)(dcm::Orientation)(dcm::Angle)(dcm::Coincidence)(dcm::Alignment)
// 
// #define LEVEL_1(r, state) \
//       template void dcm::detail::Constraint<BOOST_PP_SEQ_ELEM(0,state), BOOST_PP_SEQ_ELEM(1,state)>::initialize(BOOST_PP_SEQ_ELEM(2,state)&); \
//       
// #define LEVEL_1_OP(r, state) \
//       BOOST_PP_SEQ_REMOVE(state,2) 
//       
// #define LEVEL_1_PRED(r, state) \
//       BOOST_PP_LESS(2,BOOST_PP_SEQ_SIZE(state))
//       
// #define INITIALIZE(System, Dim) \
//       BOOST_PP_FOR(BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_SEQ_PUSH_FRONT(SEQUENCE, Dim), System), LEVEL_1_PRED, LEVEL_1_OP, LEVEL_1)  
// 
// #endif //external
*/

namespace dcm {

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
boost::shared_ptr<Derived> Module3D<Typelist, ID>::type<Sys>::Constraint3D_base<Derived>::clone(Sys& newSys) {

    //copy the standart stuff
    boost::shared_ptr<Derived> np = boost::shared_ptr<Derived>(new Derived(*static_cast<Derived*>(this)));
    np->m_system = &newSys;
    //copy the internals
    np->content = CBase::content->clone();
    //and get the geometry pointers right
    if(CBase::first) {
        GlobalVertex v = boost::static_pointer_cast<Geometry3D>(CBase::first)->template getProperty<vertex_prop>();
        np->first = newSys.m_cluster->template getObject<Geometry3D>(v);
    }
    if(CBase::second) {
        GlobalVertex v = boost::static_pointer_cast<Geometry3D>(CBase::second)->template getProperty<vertex_prop>();
        np->second = newSys.m_cluster->template getObject<Geometry3D>(v);
    }
    return np;
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::Constraint3D_id(Sys& system, Geom f, Geom s)
    : Constraint3D_base<Derived>(system, f, s) {

};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
typename Module3D<Typelist, ID>::template type<Sys>::Identifier&
Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::getIdentifier() {
    return  this->template getProperty<id_prop<Identifier> >();
};

template<typename Typelist, typename ID>
template<typename Sys>
template<typename Derived>
void Module3D<Typelist, ID>::type<Sys>::Constraint3D_id<Derived>::setIdentifier(Identifier id) {
    this->template setProperty<id_prop<Identifier> >(id);
};

template<typename Typelist, typename ID>
template<typename Sys>
Module3D<Typelist, ID>::type<Sys>::Constraint3D::Constraint3D(Sys& system, Geom first, Geom second)
    : mpl::if_<boost::is_same<Identifier, No_Identifier>,
      Constraint3D_base<Constraint3D>,
      Constraint3D_id<Constraint3D> >::type(system, first, second) {
};

} //dcm

#endif //DCM_MODULE_3D_IMP_H
