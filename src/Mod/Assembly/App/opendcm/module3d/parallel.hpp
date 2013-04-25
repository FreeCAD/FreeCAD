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

using boost::math::isnormal;

namespace dcm {

//the calculations( same as we always calculate directions we can outsource the work to this functions)
namespace parallel_detail {

template<typename Kernel, typename T>
inline typename Kernel::number_type calc(T d1,
        T d2,
        Direction dir)  {

    switch(dir) {
        case Same:
            return (d1-d2).norm();
        case Opposite:
            return (d1+d2).norm();
        case Both:
            if(d1.dot(d2) >= 0) {
                return (d1-d2).norm();
            }
            return (d1+d2).norm();
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
        case Same:
            res = ((d1-d2).dot(dd1) / (d1-d2).norm());
            break;
        case Opposite:
            res= ((d1+d2).dot(dd1) / (d1+d2).norm());
            break;
        case Both:
            if(d1.dot(d2) >= 0) {
                res = (((d1-d2).dot(dd1) / (d1-d2).norm()));
                break;
            }
            res = (((d1+d2).dot(dd1) / (d1+d2).norm()));
            break;
    }
    if((isnormal)(res)) return res;
    return 0;
};

template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradSecond(T d1,
        T d2,
        T dd2,
        Direction dir)  {

    typename Kernel::number_type res;
    switch(dir) {
        case Same:
            res = ((d1-d2).dot(-dd2) / (d1-d2).norm());
            break;
        case Opposite:
            res = ((d1+d2).dot(dd2) / (d1+d2).norm());
            break;
        case Both:
            if(d1.dot(d2) >= 0) {
                res = (((d1-d2).dot(-dd2) / (d1-d2).norm()));
                break;
            }
            res = (((d1+d2).dot(dd2) / (d1+d2).norm()));
            break;
    }
    if((isnormal)(res)) return res;
    return 0;
};

template<typename Kernel, typename T>
inline void calcGradFirstComp(T d1,
                              T d2,
                              T grad,
                              Direction dir)  {

    switch(dir) {
        case Same:
            grad = (d1-d2) / (d1-d2).norm();
            return;
        case Opposite:
            grad = (d1+d2) / (d1+d2).norm();
            return;
        case Both:
            assert(false);
    }
};

template<typename Kernel, typename T>
inline void calcGradSecondComp(T d1,
                               T d2,
                               T grad,
                               Direction dir)  {

    switch(dir) {
        case Same:
            grad = (d2-d1) / (d1-d2).norm();
            return;
        case Opposite:
            grad = (d2+d1) / (d1+d2).norm();
            return;
        case Both:
            assert(false);
    }
};

}

template< typename Kernel >
struct Parallel::type< Kernel, tag::line3D, tag::line3D > : public dcm::PseudoScale<Kernel> {

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        Direction value;

        //template definition
        Scalar calculate(Vector& param1,  Vector& param2) {
            return parallel_detail::calc<Kernel>(param1.template tail<3>(), param2.template tail<3>(), value);
        };
        Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
            return parallel_detail::calcGradFirst<Kernel>(param1.template tail<3>(), param2.template tail<3>(), dparam1.template tail<3>(), value);
        };
        Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
            return parallel_detail::calcGradSecond<Kernel>(param1.template tail<3>(), param2.template tail<3>(), dparam2.template tail<3>(), value);
        };
        void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
            gradient.template head<3>().setZero();
            parallel_detail::calcGradFirstComp<Kernel>(param1.template tail<3>(), param2.template tail<3>(), gradient.template tail<3>(), value);
        };
        void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
            gradient.template head<3>().setZero();
            parallel_detail::calcGradSecondComp<Kernel>(param1.template tail<3>(), param2.template tail<3>(), gradient.template tail<3>(), value);
        };
    };

template< typename Kernel >
struct Parallel::type< Kernel, tag::cylinder3D, tag::cylinder3D >  : public dcm::PseudoScale<Kernel>{

        typedef typename Kernel::number_type Scalar;
        typedef typename Kernel::VectorMap   Vector;

        Direction value;
	
        Scalar calculate(Vector& param1,  Vector& param2) {
            return parallel_detail::calc<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), value);
        };
        Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
            return parallel_detail::calcGradFirst<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3),
                                                   dparam1.template segment<3>(3), value);

        };
        Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
            return parallel_detail::calcGradSecond<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3),
                                                    dparam2.template segment<3>(3), value);
        };
        void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
            gradient.template head<3>().setZero();
            parallel_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3),
                                                gradient.template segment<3>(3), value);
        };
        void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
            gradient.template head<3>().setZero();
            parallel_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3),
                                                 gradient.template segment<3>(3), value);
        };
    };

template< typename Kernel >
struct Parallel::type< Kernel, tag::line3D, tag::plane3D > : public Parallel::type<Kernel, tag::line3D, tag::line3D> {};

template< typename Kernel >
struct Parallel::type< Kernel, tag::plane3D, tag::plane3D > : public Parallel::type<Kernel, tag::line3D, tag::line3D> {};

}

#endif //GCM_ANGLE
