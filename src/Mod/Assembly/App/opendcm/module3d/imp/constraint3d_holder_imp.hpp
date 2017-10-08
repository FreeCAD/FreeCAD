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

#ifndef DCM_CONSTRAINT_3D_HOLDER_IMP_H
#define DCM_CONSTRAINT_3D_HOLDER_IMP_H

#include "../geometry.hpp"
#include "opendcm/core/imp/constraint_holder_imp.hpp"

#ifdef DCM_EXTERNAL_CORE
//following macros are used for externalization. As the holder type can hold a very big set of combinations,
//especially with module states recursive incarnation, we explicitly initiate all possible versions of this
//struct so that it must only be compiled once. To get all possible equation
//combinations boost pp is needed.


template<typename T>
struct fusion_vec {
    typedef typename mpl::if_< mpl::is_sequence<T>, T, fusion::vector1<T> >::type type;
};

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/for.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/push_front.hpp>
#include <boost/preprocessor/seq/remove.hpp>

#define TAG_SEQUENCE (dcm::tag::direction3D)(dcm::tag::point3D)(dcm::tag::line3D)(dcm::tag::plane3D)(dcm::tag::cylinder3D)
#define CONSTRAINT_SEQUENCE (dcm::Distance)(dcm::Orientation)(dcm::Angle)(dcm::Coincidence)(dcm::Alignment)

#define LEVEL_2_TYPE(r, state) \
      template struct dcm::detail::Constraint<BOOST_PP_SEQ_ELEM(0,state), BOOST_PP_SEQ_ELEM(1,state)>::holder<fusion_vec<BOOST_PP_SEQ_ELEM(2,state)>::type, BOOST_PP_SEQ_ELEM(3,state), BOOST_PP_SEQ_ELEM(4,state)>;
      
#define LEVEL_2_TYPE_OP(r, state) \
      BOOST_PP_SEQ_REMOVE(state,4)
      
#define LEVEL_2_TYPE_PRED(r, state) \
      BOOST_PP_LESS(4,BOOST_PP_SEQ_SIZE(state))

#define LEVEL_1_TYPE(r, state) \
      template struct dcm::detail::Constraint<BOOST_PP_SEQ_ELEM(0,state), BOOST_PP_SEQ_ELEM(1,state)>::holder<fusion_vec<BOOST_PP_SEQ_ELEM(2,state)>::type, BOOST_PP_SEQ_ELEM(3,state), BOOST_PP_SEQ_ELEM(3,state)>;\
      BOOST_PP_FOR(state, LEVEL_2_TYPE_PRED, LEVEL_2_TYPE_OP, LEVEL_2_TYPE)
      
#define LEVEL_1_TYPE_OP(r, state) \
      BOOST_PP_SEQ_REMOVE(state,3) 
      
#define LEVEL_1_TYPE_PRED(r, state) \
      BOOST_PP_LESS(3,BOOST_PP_SEQ_SIZE(state))
      
#define INITIALIZE_TYPES(System, Dim, Covec, SEQUENCE) \
      BOOST_PP_FOR(BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_SEQ_PUSH_FRONT(SEQUENCE, Covec), Dim), System), LEVEL_1_TYPE_PRED, LEVEL_1_TYPE_OP, LEVEL_1_TYPE)  


#define LEVEL_1(r, state) \
      INITIALIZE_TYPES(BOOST_PP_SEQ_ELEM(0,state), BOOST_PP_SEQ_ELEM(1,state), BOOST_PP_SEQ_ELEM(2,state), TAG_SEQUENCE) \
      
#define LEVEL_1_OP(r, state) \
      BOOST_PP_SEQ_REMOVE(state,2) 
      
#define LEVEL_1_PRED(r, state) \
      BOOST_PP_LESS(2,BOOST_PP_SEQ_SIZE(state))
      
#define INITIALIZE(System, Dim, SEQUENCE) \
      BOOST_PP_FOR(BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_SEQ_PUSH_FRONT(SEQUENCE, Dim), System), LEVEL_1_PRED, LEVEL_1_OP, LEVEL_1)  

#endif //external

#endif