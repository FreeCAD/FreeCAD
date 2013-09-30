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

#ifndef GCM_ANGLE_HPP
#define GCM_ANGLE_HPP

#include "geometry.hpp"
#include <opendcm/core/equations.hpp>

namespace dcm {

//the calculations( same as we always calculate directions we can outsource the work to this functions)
namespace angle_detail {

template<typename Kernel, typename T>
inline typename Kernel::number_type calc(const T& d1,
        const T& d2,
        const typename Kernel::number_type& angle)  {

    return d1.dot(d2) / (d1.norm()*d2.norm()) - std::cos(angle);
};


template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradFirst(const T& d1,
        const T& d2,
        const T& dd1)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  dd1.dot(d2)/norm - (d1.dot(d2) * (d1.dot(dd1)/d1.norm())*d2.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T>
inline typename Kernel::number_type calcGradSecond(const T& d1,
        const T& d2,
        const T& dd2)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  d1.dot(dd2)/norm - (d1.dot(d2) * (d2.dot(dd2)/d2.norm())*d1.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T>
inline void calcGradFirstComp(const T& d1,
                              const T& d2,
                              const T& grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    const_cast< T& >(grad) = d2/norm - d1.dot(d2)*d1/(std::pow(d1.norm(),3)*d2.norm());
};

template<typename Kernel, typename T>
inline void calcGradSecondComp(const T& d1,
                               const T& d2,
                               const T& grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    const_cast< T& >(grad) = d1/norm - d1.dot(d2)*d2/(std::pow(d2.norm(),3)*d1.norm());
};

}


template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::line3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    option_type value;

    //template definition
    Scalar calculate(Vector& param1,  Vector& param2) {
        return angle_detail::calc<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), value);
    };
    Scalar calculateGradientFirst(Vector& param1, Vector& param2, Vector& dparam1) {
        return angle_detail::calcGradFirst<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), dparam1.template segment<3>(3));
    };
    Scalar calculateGradientSecond(Vector& param1, Vector& param2, Vector& dparam2) {
        return angle_detail::calcGradSecond<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), dparam2.template segment<3>(3));
    };
    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        angle_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), gradient.template segment<3>(3));
    };
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        gradient.template head<3>().setZero();
        angle_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), gradient.template segment<3>(3));
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::plane3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {};

template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::plane3D, tag::plane3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {};

template< typename Kernel >
struct Angle::type< Kernel, tag::plane3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    void calculateGradientFirstComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientFirstComplete(param1, param2, gradient);
        gradient(6)=0;
    };
    
    void calculateGradientSecondComplete(Vector& param1, Vector& param2, Vector& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

}

#endif //GCM_ANGLE_HPP
