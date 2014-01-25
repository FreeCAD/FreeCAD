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

template<typename Kernel, typename T1, typename T2>
inline typename Kernel::number_type calc(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const typename Kernel::number_type& angle)  {

    return d1.dot(d2) / (d1.norm()*d2.norm()) - std::cos(angle);
};


template<typename Kernel, typename T1, typename T2, typename T3>
inline typename Kernel::number_type calcGradFirst(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const E::MatrixBase<T3>& dd1)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  dd1.dot(d2)/norm - (d1.dot(d2) * (d1.dot(dd1)/d1.norm())*d2.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline typename Kernel::number_type calcGradSecond(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const E::MatrixBase<T3>& dd2)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    return  d1.dot(dd2)/norm - (d1.dot(d2) * (d2.dot(dd2)/d2.norm())*d1.norm()) / std::pow(norm,2);
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline void calcGradFirstComp(const E::MatrixBase<T1>& d1,
                              const E::MatrixBase<T2>& d2,
                              const E::MatrixBase<T3>& grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    const_cast< E::MatrixBase<T3>& >(grad) = d2/norm - d1.dot(d2)*d1/(std::pow(d1.norm(),3)*d2.norm());
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline void calcGradSecondComp(const E::MatrixBase<T1>& d1,
                               const E::MatrixBase<T2>& d2,
                               const E::MatrixBase<T3>& grad)  {

    typename Kernel::number_type norm = d1.norm()*d2.norm();
    const_cast< E::MatrixBase<T3>& >(grad) = d1/norm - d1.dot(d2)*d2/(std::pow(d2.norm(),3)*d1.norm());
};

}


template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::line3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    typename Angle::options values;

    //template definition
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return angle_detail::calc<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), fusion::at_key<double>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return angle_detail::calcGradFirst<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), dparam1.template segment<3>(3));
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return angle_detail::calcGradSecond<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), dparam2.template segment<3>(3));
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        angle_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), gradient.template segment<3>(3));
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        angle_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3), param2.template segment<3>(3), gradient.template segment<3>(3));
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::plane3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {};

template< typename Kernel >
struct Angle::type< Kernel, tag::line3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::plane3D, tag::plane3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {};

template< typename Kernel >
struct Angle::type< Kernel, tag::plane3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Angle::type< Kernel, tag::cylinder3D, tag::cylinder3D > : public Angle::type<Kernel, tag::line3D, tag::line3D> {

    typedef typename Kernel::VectorMap   Vector;

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientFirstComplete(param1, param2, gradient);
        gradient(6)=0;
    };

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        Angle::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

}

#endif //GCM_ANGLE_HPP
