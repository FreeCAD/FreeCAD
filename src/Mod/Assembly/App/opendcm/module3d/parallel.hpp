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

#ifndef GCM_PARALLEL_H
#define GCM_PARALLEL_H

#include <opendcm/core/constraint.hpp>

#include "geometry.hpp"
#include <boost/math/special_functions/fpclassify.hpp>

using boost::math::isfinite;

namespace dcm {

//the calculations( same as we always calculate directions we can outsource the work to this functions)
namespace orientation_detail {

template<typename Kernel, typename T>
inline typename Kernel::number_type calc(T d1,
        T d2,
        Direction dir)  {

    switch(dir) {
        case equal:
            return (d1-d2).norm();
        case opposite:
            return (d1+d2).norm();
        case parallel:
            return d1.dot(d2) - d1.norm()*d2.norm();
        case perpendicular:
            return d1.dot(d2);
        default:
            assert(false);
    }
    return 0;
};


template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradFirst(T d1,
        T d2,
        T dd1,
        Direction dir)  {

    typename Kernel::number_type res;
    switch(dir) {
        case equal:
            res = ((d1-d2).dot(dd1) / (d1-d2).norm());
            break;
        case opposite:
            res= ((d1+d2).dot(dd1) / (d1+d2).norm());
            break;
        case parallel:
            res = dd1.dot(d2) - d1.dot(dd1)/d1.norm()*d2.norm();
            break;
        case perpendicular:
            res = dd1.dot(d2);
            break;
    }
    if(isfinite(res)) return res;

    return 0;
};

template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradSecond(T d1,
        T d2,
        T dd2,
        Direction dir)  {

    typename Kernel::number_type res;
    switch(dir) {
        case equal:
            res = ((d1-d2).dot(-dd2) / (d1-d2).norm());
            break;
        case opposite:
            res = ((d1+d2).dot(dd2) / (d1+d2).norm());
            break;
        case parallel:
            res = d1.dot(dd2) - d2.dot(dd2)/d2.norm()*d1.norm();
            break;
        case perpendicular:
            res = d1.dot(dd2);
            break;
    }
    if((isfinite)(res)) return res;
    return 0;
};

template<typename Kernel, typename T>
inline void calcGradFirstComp(T d1,
                              T d2,
                              T grad,
                              Direction dir)  {

    switch(dir) {
        case equal:
            grad = (d1-d2) / (d1-d2).norm();
            return;
        case opposite:
            grad = (d1+d2) / (d1+d2).norm();
            return;
        case parallel:
            grad = d2 - d1/d1.norm()*d2.norm();
            return;
        case perpendicular:
            grad = d2;
            return;
    }
};

template<typename Kernel, typename T>
inline void calcGradSecondComp(T d1,
                               T d2,
                               T grad,
                               Direction dir)  {

    switch(dir) {
        case equal:
            grad = (d2-d1) / (d1-d2).norm();
            return;
        case opposite:
            grad = (d2+d1) / (d1+d2).norm();
            return;
        case parallel:
            grad = d1 - d2/d2.norm()*d1.norm();
            return;
        case perpendicular:
            grad = d1;
            return;
    }
};

}

template< typename Kernel >
struct Orientation::type< Kernel, tag::direction3D, tag::direction3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    option_type value;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return orientation_detail::calc<Kernel>(param1, param2, value);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1, param2, dparam1, value);
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1, param2, dparam2, value);
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1, param2, gradient, value);
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1, param2, gradient, value);
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::line3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    option_type value;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return orientation_detail::calc<Kernel>(param1.template segment<3>(3),
                                                param2.template segment<3>(3),
                                                value);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam1.template segment<3>(3),
                value);
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam2.template segment<3>(3),
                value);
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                value);
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                value);
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::plane3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    option_type value;

    option_type getValue() {
        if(value==parallel)
            return perpendicular;
        if(value==perpendicular)
            return parallel;
        return value;
    };

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return orientation_detail::calc<Kernel>(param1.template segment<3>(3),
                                                param2.template segment<3>(3),
                                                getValue());
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam1.template segment<3>(3),
                getValue());
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam2.template segment<3>(3),
                getValue());
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                getValue());
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                getValue());
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::cylinder3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {};

template< typename Kernel >
struct Orientation::type< Kernel, tag::plane3D, tag::plane3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {};

template< typename Kernel >
struct Orientation::type< Kernel, tag::plane3D, tag::cylinder3D > : public Orientation::type<Kernel, tag::line3D, tag::plane3D> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    using Orientation::type<Kernel, tag::line3D, tag::plane3D>::value;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculate(param1, param2);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientFirst(param1, param2, dparam1);
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientSecond(param1, param2, dparam2);
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientFirstComplete(param1, param2, gradient);
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D >  : public Orientation::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::VectorMap   Vector;

    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                Orientation::type< Kernel, tag::line3D, tag::line3D >::value);
        gradient(6) = 0;
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                Orientation::type< Kernel, tag::line3D, tag::line3D >::value);
        gradient(6) = 0;
    };
};
}

#endif 
