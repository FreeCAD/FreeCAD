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
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_CONSTRAINT_IMP_H
#define DCM_CONSTRAINT_IMP_H

#include "../constraint.hpp"

#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace detail {

  

template<typename Sys, int Dim>
template<typename ConstraintVector>
void Constraint<Sys, Dim>::initialize(ConstraintVector& cv) {

    //use the compile time unrolling to retrieve the geometry tags
    initializeFirstGeometry<mpl::int_<0> >(cv, mpl::true_());
};

template<typename Sys, int Dim>
template<typename WhichType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFirstGeometry(ConstraintVector& cv, boost::mpl::false_ /*unrolled*/) {
    //this function is only for breaking the compilation loop, it should never be called
    BOOST_ASSERT(false); //Should never assert here; only meant to stop recursion at the end of the typelist
};

template<typename Sys, int Dim>
template<typename WhichType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFirstGeometry(ConstraintVector& cv, boost::mpl::true_ /*unrolled*/) {

    typedef typename Sys::geometries geometries;

    switch(first->getExactType()) {

#ifdef BOOST_PP_LOCAL_ITERATE
#define BOOST_PP_LOCAL_MACRO(n) \
      case (WhichType::value + n): \
        return initializeSecondGeometry<boost::mpl::int_<0>,\
		typename mpl::at<geometries, typename in_range_value<geometries, WhichType::value + n>::type >::type,\
					ConstraintVector>(cv, typename boost::mpl::less<boost::mpl::int_<WhichType::value + n>, boost::mpl::size<geometries> >::type()); \
        break;
#define BOOST_PP_LOCAL_LIMITS (0, 10)
#include BOOST_PP_LOCAL_ITERATE()
#endif //BOOST_PP_LOCAL_ITERATE
    default:
        typedef typename mpl::int_<WhichType::value + 10> next_which_t;
        return initializeFirstGeometry<next_which_t, ConstraintVector> (cv,
                typename mpl::less< next_which_t, typename mpl::size<geometries>::type >::type());
    }
};

template<typename Sys, int Dim>
template<typename WhichType, typename FirstType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeSecondGeometry(ConstraintVector& cv, boost::mpl::false_ /*unrolled*/) {
    //this function is only for breaking the compilation loop, it should never be called
    BOOST_ASSERT(false); //Should never assert here; only meant to stop recursion at the end of the typelist
};

template<typename Sys, int Dim>
template<typename WhichType, typename FirstType, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeSecondGeometry(ConstraintVector& cv, boost::mpl::true_ /*unrolled*/) {

    typedef typename Sys::geometries geometries;
    switch(second->getExactType()) {

#ifdef BOOST_PP_LOCAL_ITERATE
#define BOOST_PP_LOCAL_MACRO(n) \
      case (WhichType::value + n): \
        return intitalizeFinalize<FirstType, \
		typename mpl::at<geometries, typename in_range_value<geometries, WhichType::value + n>::type >::type,\
				  ConstraintVector>(cv, typename boost::mpl::less<boost::mpl::int_<WhichType::value + n>, boost::mpl::size<geometries> >::type()); \
        break;
#define BOOST_PP_LOCAL_LIMITS (0, 10)
#include BOOST_PP_LOCAL_ITERATE()
#endif //BOOST_PP_LOCAL_ITERATE
    default:
        typedef typename mpl::int_<WhichType::value + 10> next_which_t;
        return initializeSecondGeometry<next_which_t, FirstType, ConstraintVector>
               (cv, typename mpl::less
                < next_which_t
                , typename mpl::size<geometries>::type>::type()
               );
    }
};

template<typename Sys, int Dim>
template<typename FirstType, typename SecondType, typename ConstraintVector>
inline void Constraint<Sys, Dim>::intitalizeFinalize(ConstraintVector& cv, boost::mpl::true_ /*is_unrolled_t*/) {

    initializeFromTags<FirstType, SecondType>(cv);
};

template<typename Sys, int Dim>
template<typename FirstType, typename SecondType, typename ConstraintVector>
inline void Constraint<Sys, Dim>::intitalizeFinalize(ConstraintVector& cv, boost::mpl::false_ /*is_unrolled_t*/) {
    //Should never be here at runtime; only required to block code generation that deref's the sequence out of bounds
    BOOST_ASSERT(false);
}

template<typename Sys, int Dim>
template<typename tag1, typename tag2, typename ConstraintVector>
void Constraint<Sys, Dim>::initializeFromTags(ConstraintVector& v) {

    typedef tag_order< tag1, tag2 > order;

    //and build the placeholder
    content = new holder<ConstraintVector, typename order::first_tag, typename order::second_tag >(v);

    //geometry order needs to be the one needed by equations
    if(order::swapt::value)
        first.swap(second);
};

template<typename Sys, int Dim>
Constraint<Sys, Dim>::Constraint(geom_ptr f, geom_ptr s)
    : first(f), second(s), content(0)	{

    //cf = first->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
    //cs = second->template connectSignal<reset> (boost::bind(&Constraint::geometryReset, this, _1));
};

template<typename Sys, int Dim>
Constraint<Sys, Dim>::~Constraint()  {
    delete content;
    //first->template disconnectSignal<reset>(cf);
    //second->template disconnectSignal<reset>(cs);
};

template<typename Sys, int Dim>
int Constraint<Sys, Dim>::equationCount() {
    return content->equationCount();
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::calculate(Scalar scale, AccessType access) {
    content->calculate(first, second, scale, access);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::treatLGZ() {
    content->treatLGZ(first, second);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::setMaps(MES& mes) {
    content->setMaps(mes, first, second);
};

template<typename Sys, int Dim>
void Constraint<Sys, Dim>::collectPseudoPoints(Vec& vec1, Vec& vec2) {
    content->collectPseudoPoints(first, second, vec1, vec2);
};

template<typename Sys, int Dim>
std::vector<boost::any> Constraint<Sys, Dim>::getGenericEquations() {
    return content->getGenericEquations();
};

template<typename Sys, int Dim>
std::vector<boost::any> Constraint<Sys, Dim>::getGenericConstraints() {
    return content->getGenericConstraints();
};

template<typename Sys, int Dim>
std::vector<const std::type_info*> Constraint<Sys, Dim>::getEquationTypes() {
    return content->getEquationTypes();
};

template<typename Sys, int Dim>
std::vector<const std::type_info*> Constraint<Sys, Dim>::getConstraintTypes() {
    return content->getConstraintTypes();
};

};//detail

};//dcm

#endif //GCM_CONSTRAINT_H



