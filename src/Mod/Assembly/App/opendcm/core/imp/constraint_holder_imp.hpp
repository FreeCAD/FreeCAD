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

#ifndef DCM_CONSTRAINT_HOLDER_IMP_H
#define DCM_CONSTRAINT_HOLDER_IMP_H

#include "opendcm/core/constraint.hpp"

#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

namespace dcm {

namespace detail {


template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::OptionSetter::OptionSetter(Objects& val) : objects(val) {};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template<typename T>
typename boost::enable_if<typename Constraint<Sys, Dim>::template holder<ConstraintVector, tag1, tag2>::template has_option<T>::type, void>::type
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::OptionSetter::operator()(EquationSet<T>& val) const {

    //get the index of the corresbonding equation
    typedef typename mpl::find<EquationVector, T>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
    BOOST_MPL_ASSERT((mpl::not_<boost::is_same<iterator, typename mpl::end<EquationVector>::type > >));
    fusion::copy(fusion::at<distance>(objects).values, val.m_eq.values);
    val.access = fusion::at<distance>(objects).access;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template<typename T>
typename boost::enable_if<mpl::not_<typename Constraint<Sys, Dim>::template holder<ConstraintVector, tag1, tag2>::template has_option<T>::type>, void>::type
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::OptionSetter::operator()(EquationSet<T>& val) const {
    typedef typename mpl::find<EquationVector, T>::type iterator;
    typedef typename mpl::distance<typename mpl::begin<EquationVector>::type, iterator>::type distance;
    val.access = fusion::at<distance>(objects).access;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::Calculater::Calculater(geom_ptr f, geom_ptr s, Scalar sc, AccessType a)
    : first(f), second(s), scale(sc), access(a) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::Calculater::operator()(T& val) const {

    //if the equation is disabled we don't have anything mapped so avoid accessing it
    if(!val.enabled)
        return;

    //if we are not one of the accessed types we don't need to be recalculated
    if((access==rotation && val.access!=rotation) 
	|| (access == general && val.access != general)) {

        val.m_residual(0) = 0;
        if(first->getClusterMode()) {
            if(!first->isClusterFixed()) {
                val.m_diff_first_rot.setZero();
                val.m_diff_first.setZero();
            }
        }
        else
            val.m_diff_first.setZero();

        if(second->getClusterMode()) {
            if(!second->isClusterFixed()) {
                val.m_diff_second_rot.setZero();
                val.m_diff_second.setZero();
            }
        }
        else
            val.m_diff_second.setZero();

    }
    //we need to calculate, so lets go for it!
    else {

        val.m_eq.setScale(scale);

        val.m_residual(0) = val.m_eq.calculate(first->m_parameter, second->m_parameter);

        //now see which way we should calculate the gradient (may be different for both geometries)
        if(first->m_parameterCount) {
            if(first->getClusterMode()) {
                //when the cluster is fixed no maps are set as no parameters exist.
                if(!first->isClusterFixed()) {

                    //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                    for(int i=0; i<3; i++) {
                        val.m_diff_first_rot(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                  second->m_parameter, first->m_diffparam.col(i));
                    }
                    //and now with the translations
                    for(int i=0; i<3; i++) {
                        val.m_diff_first(i) = val.m_eq.calculateGradientFirst(first->m_parameter,
                                              second->m_parameter, first->m_diffparam.col(i+3));
                    }
                }
            }
            else {
                //not in cluster, so allow the constraint to optimize the gradient calculation
                val.m_eq.calculateGradientFirstComplete(first->m_parameter, second->m_parameter, val.m_diff_first);
            }
        }
        if(second->m_parameterCount) {
            if(second->getClusterMode()) {
                if(!second->isClusterFixed()) {

                    //cluster mode, so we do a full calculation with all 3 rotation diffparam vectors
                    for(int i=0; i<3; i++) {
                        val.m_diff_second_rot(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                                   second->m_parameter, second->m_diffparam.col(i));
                    }
                    //and the translation separated
                    for(int i=0; i<3; i++) {
                        val.m_diff_second(i) = val.m_eq.calculateGradientSecond(first->m_parameter,
                                               second->m_parameter, second->m_diffparam.col(i+3));
                    }
                }
            }
            else {
                //not in cluster, so allow the constraint to optimize the gradient calculation
                val.m_eq.calculateGradientSecondComplete(first->m_parameter, second->m_parameter, val.m_diff_second);
            }
        }
    }
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::MapSetter::MapSetter(MES& m, geom_ptr f, geom_ptr s)
    : mes(m), first(f), second(s) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::MapSetter::operator()(T& val) const {

    if(!val.enabled)
        return;

    //when in cluster, there are 6 clusterparameter we differentiat for, if not we differentiat
    //for every parameter in the geometry;
    int equation = mes.setResidualMap(val.m_residual, val.access);
    
    if(first->getClusterMode()) {
        if(!first->isClusterFixed()) {
            mes.setJacobiMap(equation, first->m_offset_rot, 3, val.m_diff_first_rot);
            mes.setJacobiMap(equation, first->m_offset, 3, val.m_diff_first);
        }
    }
    else
        mes.setJacobiMap(equation, first->m_offset, first->m_parameterCount, val.m_diff_first);


    if(second->getClusterMode()) {
        if(!second->isClusterFixed()) {
            mes.setJacobiMap(equation, second->m_offset_rot, 3, val.m_diff_second_rot);
            mes.setJacobiMap(equation, second->m_offset, 3, val.m_diff_second);
        }
    }
    else
        mes.setJacobiMap(equation, second->m_offset, second->m_parameterCount, val.m_diff_second);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::PseudoCollector::PseudoCollector(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2)
    : first(f), second(s), points1(vec1), points2(vec2) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::PseudoCollector::operator()(T& val) const {

    if(!val.enabled)
        return;

    if(first->m_isInCluster && second->m_isInCluster) {
        val.m_eq.calculatePseudo(first->m_rotated, points1, second->m_rotated, points2);
    }
    else if(first->m_isInCluster) {
        typename Kernel::Vector sec = second->m_parameter;
        val.m_eq.calculatePseudo(first->m_rotated, points1, sec, points2);
    }
    else if(second->m_isInCluster) {
        typename Kernel::Vector fir = first->m_parameter;
        val.m_eq.calculatePseudo(fir, points1, second->m_rotated, points2);
    }
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::LGZ::LGZ(geom_ptr f, geom_ptr s)
    : first(f), second(s) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::LGZ::operator()(T& val) const {

    typedef typename Sys::Kernel Kernel;

    if(!val.enabled)
        return;

    //to treat local gradient zeros we calculate a approximate second derivative of the equations
    //only do that if neseccary: residual is not zero
    if(!Kernel::isSame(val.m_residual(0),0, 1e-7)) { //TODO: use exact precission and scale value

        //rotations exist only in cluster
        if(first->getClusterMode() && !first->isClusterFixed()) {
            //LGZ exists for rotations only
            for(int i=0; i<3; i++) {

                //only treat if the gradient really is zero
                if(Kernel::isSame(val.m_diff_first_rot(i), 0, 1e-7)) {

                    //to get the approximated second derivative we need the slightly moved geometrie
                    const typename Kernel::Vector  p_old =  first->m_parameter;
                    first->m_parameter += first->m_diffparam.col(i)*1e-3;
                    first->normalize();
                    //with this changed geometrie we test if a gradient exist now
                    typename Kernel::VectorMap block(&first->m_diffparam(0,i),first->m_parameterCount,1, DS(1,1));
                    typename Kernel::number_type res = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                       second->m_parameter, block);
                    first->m_parameter = p_old;

                    //let's see if the initial LGZ was a real one
                    if(!Kernel::isSame(res, 0, 1e-7)) {

                        //is a fake zero, let's correct it
                        val.m_diff_first_rot(i) = res;
                    };
                };
            };
        }
        //and the same for the second one too
        if(second->getClusterMode() && !second->isClusterFixed()) {

            for(int i=0; i<3; i++) {

                //only treat if the gradient really is zero
                if(Kernel::isSame(val.m_diff_second_rot(i), 0, 1e-7)) {

                    //to get the approximated second derivative we need the slightly moved geometrie
                    const typename Kernel::Vector  p_old =  second->m_parameter;
                    second->m_parameter += second->m_diffparam.col(i)*1e-3;
                    second->normalize();
                    //with this changed geometrie we test if a gradient exist now
                    typename Kernel::VectorMap block(&second->m_diffparam(0,i),second->m_parameterCount,1, DS(1,1));
                    typename Kernel::number_type res = val.m_eq.calculateGradientFirst(first->m_parameter,
                                                       second->m_parameter, block);
                    second->m_parameter = p_old;

                    //let's see if the initial LGZ was a real one
                    if(!Kernel::isSame(res, 0, 1e-7)) {

                        //is a fake zero, let's correct it
                        val.m_diff_second_rot(i) = res;
                    };
                };
            };
        };
    };
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::GenericEquations::GenericEquations(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::GenericEquations::operator()(T& val) const {
    vec.push_back(val.m_eq);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::GenericConstraints::GenericConstraints(std::vector<boost::any>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::GenericConstraints::operator()(T& val) const {
    vec.push_back(val);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::Types::Types(std::vector<const std::type_info*>& v)
    : vec(v) {

};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
template< typename T >
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::Types::operator()(T& val) const {
    vec.push_back(&typeid(T));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::holder(Objects& obj) : m_objects(obj)  {
    //set the initial values in the equations
    fusion::for_each(m_sets, OptionSetter(obj));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::calculate(geom_ptr first, geom_ptr second,
        Scalar scale, AccessType access) {
    fusion::for_each(m_sets, Calculater(first, second, scale, access));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::treatLGZ(geom_ptr first, geom_ptr second) {
    fusion::for_each(m_sets, LGZ(first, second));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
typename Constraint<Sys, Dim>::placeholder*
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::resetConstraint(geom_ptr first, geom_ptr second) const {
    //boost::apply_visitor(creator, first->m_geometry, second->m_geometry);
    //if(creator.need_swap) first.swap(second);
    return NULL;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::setMaps(MES& mes, geom_ptr first, geom_ptr second) {
    fusion::for_each(m_sets, MapSetter(mes, first, second));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::collectPseudoPoints(geom_ptr f, geom_ptr s, Vec& vec1, Vec& vec2) {
    fusion::for_each(m_sets, PseudoCollector(f, s, vec1, vec2));
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
typename Constraint<Sys, Dim>::placeholder*
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::clone() {
    return new holder(*this);
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
int Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::equationCount() {
    int count = 0;
    EquationCounter counter(count);
    fusion::for_each(m_sets, counter);
    return count;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
std::vector<boost::any>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::getGenericEquations() {
    std::vector<boost::any> vec;
    fusion::for_each(m_sets, GenericEquations(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
std::vector<boost::any>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::getGenericConstraints() {
    std::vector<boost::any> vec;
    fusion::for_each(m_objects, GenericConstraints(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
std::vector<const std::type_info*>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::getEquationTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< EquationVector >(Types(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
std::vector<const std::type_info*>
Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::getConstraintTypes() {
    std::vector<const std::type_info*> vec;
    mpl::for_each< ConstraintVector >(Types(vec));
    return vec;
};

template<typename Sys, int Dim>
template<typename ConstraintVector, typename tag1, typename tag2>
void Constraint<Sys, Dim>::holder<ConstraintVector, tag1, tag2>::disable() {
    fusion::for_each(m_sets, disabler());
};

};//detail

};//dcm

#endif //GCM_CONSTRAINT_H



