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

template<typename Kernel, typename T1, typename T2>
inline typename Kernel::number_type calc(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const Direction& dir)  {

    switch(dir) {
    case parallel:
        if(d1.dot(d2) < 0)
            return (d1+d2).norm();
    case equal:
        return (d1-d2).norm();
    case opposite:
        return (d1+d2).norm();
    case perpendicular:
        return d1.dot(d2);
    default
            :
        assert(false);
    }
    return 0;
};


template<typename Kernel, typename T1, typename T2, typename T3>
inline typename Kernel::number_type calcGradFirst(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const E::MatrixBase<T3>& dd1,
        const Direction& dir)  {

    typename Kernel::number_type res;
    switch(dir) {
    case parallel:
        if(d1.dot(d2) < 0) {
            res= ((d1+d2).dot(dd1) / (d1+d2).norm());
            break;
        }
    case equal:
        res = ((d1-d2).dot(dd1) / (d1-d2).norm());
        break;
    case opposite:
        res= ((d1+d2).dot(dd1) / (d1+d2).norm());
        break;
    case perpendicular:
        res = dd1.dot(d2);
        break;
    }
    if(isfinite(res))
        return res;

    return 0;
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline typename Kernel::number_type calcGradSecond(const E::MatrixBase<T1>& d1,
        const E::MatrixBase<T2>& d2,
        const E::MatrixBase<T3>& dd2,
        const Direction& dir)  {

    typename Kernel::number_type res;
    switch(dir) {
    case parallel:
        if(d1.dot(d2) < 0) {
            res = ((d1+d2).dot(dd2) / (d1+d2).norm());
            break;
        }
    case equal:
        res = ((d1-d2).dot(-dd2) / (d1-d2).norm());
        break;
    case opposite:
        res = ((d1+d2).dot(dd2) / (d1+d2).norm());
        break;
    case perpendicular:
        res = d1.dot(dd2);
        break;
    }
    if((isfinite)(res))
        return res;
    return 0;
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline void calcGradFirstComp(const E::MatrixBase<T1>& d1,
                              const E::MatrixBase<T2>& d2,
                              const E::MatrixBase<T3>& grad,
                              const Direction& dir)  {

    switch(dir) {
    case parallel:
        if(d1.dot(d2) < 0) {
            const_cast< E::MatrixBase<T3>& >(grad) = (d1+d2) / (d1+d2).norm();
            return;
        }
    case equal:
        const_cast< E::MatrixBase<T3>& >(grad) = (d1-d2) / (d1-d2).norm();
        return;
    case opposite:
        const_cast< E::MatrixBase<T3>& >(grad) = (d1+d2) / (d1+d2).norm();
        return;
    case perpendicular:
        const_cast< E::MatrixBase<T3>& >(grad) = d2;
        return;
    }
};

template<typename Kernel, typename T1, typename T2, typename T3>
inline void calcGradSecondComp(const E::MatrixBase<T1>& d1,
                               const E::MatrixBase<T2>& d2,
                               const E::MatrixBase<T3>& grad,
                               const Direction& dir)  {

    switch(dir) {
    case parallel:
        if(d1.dot(d2) < 0) {
            const_cast< E::MatrixBase<T3>& >(grad) = (d2+d1) / (d1+d2).norm();
            return;
        }
    case equal:
        const_cast< E::MatrixBase<T3>& >(grad) = (d2-d1) / (d1-d2).norm();
        return;
    case opposite:
        const_cast< E::MatrixBase<T3>& >(grad) = (d2+d1) / (d1+d2).norm();
        return;
    case perpendicular:
        const_cast< E::MatrixBase<T3>& >(grad) = d1;
        return;
    }
};

}

template< typename Kernel >
struct Orientation::type< Kernel, tag::direction3D, tag::direction3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    typename Orientation::options values;

    //template definition
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return orientation_detail::calc<Kernel>(param1, param2, fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1, param2, dparam1, fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1, param2, dparam2, fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1, param2, gradient, fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1, param2, gradient, fusion::at_key<Direction>(values).second);
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::line3D > : public dcm::PseudoScale<Kernel> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    typename Orientation::options values;

    //template definition
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return orientation_detail::calc<Kernel>(param1.template segment<3>(3),
                                                param2.template segment<3>(3),
                                                fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam1.template segment<3>(3),
                fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam2.template segment<3>(3),
                fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                fusion::at_key<Direction>(values).second);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                fusion::at_key<Direction>(values).second);
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::plane3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    options values;

    //makes it possible to allow only partial directions for derived constraints
    inline dcm::Direction getValue() {
        return fusion::at_key<Direction>(values).second;
    };

    //template definition
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return orientation_detail::calc<Kernel>(param1.template segment<3>(3),
                                                param2.template segment<3>(3),
                                                getValue());
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return orientation_detail::calcGradFirst<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam1.template segment<3>(3),
                getValue());
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return orientation_detail::calcGradSecond<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                dparam2.template segment<3>(3),
                getValue());
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                getValue());
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                getValue());
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::line3D, tag::cylinder3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        Orientation::type<Kernel, tag::line3D, tag::line3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::plane3D, tag::plane3D > : public Orientation::type< Kernel, tag::line3D, tag::line3D > {};

template< typename Kernel >
struct Orientation::type< Kernel, tag::plane3D, tag::cylinder3D > : public Orientation::type<Kernel, tag::line3D, tag::plane3D> {

    typedef typename Kernel::number_type Scalar;
    typedef typename Kernel::VectorMap   Vector;

    using Orientation::type<Kernel, tag::line3D, tag::plane3D>::values;

    //template definition
    template <typename DerivedA,typename DerivedB>
    Scalar calculate(const E::MatrixBase<DerivedA>& param1,  const E::MatrixBase<DerivedB>& param2) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculate(param1, param2);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientFirst(const E::MatrixBase<DerivedA>& param1,
                                  const E::MatrixBase<DerivedB>& param2,
                                  const E::MatrixBase<DerivedC>& dparam1) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientFirst(param1, param2, dparam1);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    Scalar calculateGradientSecond(const E::MatrixBase<DerivedA>& param1,
                                   const E::MatrixBase<DerivedB>& param2,
                                   const E::MatrixBase<DerivedC>& dparam2) {
        return Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientSecond(param1, param2, dparam2);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientFirstComplete(param1, param2, gradient);
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        Orientation::type<Kernel, tag::line3D, tag::plane3D>::calculateGradientSecondComplete(param1, param2, gradient);
        gradient(6)=0;
    };
};

template< typename Kernel >
struct Orientation::type< Kernel, tag::cylinder3D, tag::cylinder3D >  : public Orientation::type< Kernel, tag::line3D, tag::line3D > {

    typedef typename Kernel::VectorMap   Vector;

    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientFirstComplete(const E::MatrixBase<DerivedA>& param1,
                                        const E::MatrixBase<DerivedB>& param2,
                                        E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradFirstComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                fusion::at_key<dcm::Direction>(Orientation::type< Kernel, tag::line3D, tag::line3D >::values).second);
        gradient(6) = 0;
    };
    template <typename DerivedA,typename DerivedB, typename DerivedC>
    void calculateGradientSecondComplete(const E::MatrixBase<DerivedA>& param1,
                                         const E::MatrixBase<DerivedB>& param2,
                                         E::MatrixBase<DerivedC>& gradient) {
        gradient.template head<3>().setZero();
        orientation_detail::calcGradSecondComp<Kernel>(param1.template segment<3>(3),
                param2.template segment<3>(3),
                gradient.template segment<3>(3),
                fusion::at_key<dcm::Direction>(Orientation::type< Kernel, tag::line3D, tag::line3D >::values).second);
        gradient(6) = 0;
    };
};
}

#endif
